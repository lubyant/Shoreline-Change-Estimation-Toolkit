"""Water-body segmentation: DeepLabV3+ (ResNetV1c-D8) inference in plain PyTorch.

This is a minimal re-implementation of the OpenMMLab ``mmsegmentation``
model the pretrained checkpoint (see ``scet.weights``) was trained with --
``EncoderDecoder(ResNetV1c-50 d8, DepthwiseSeparableASPPHead)`` -- keeping the
same parameter names so the checkpoint's ``state_dict`` loads unchanged.
Reimplementing the forward pass means inference needs only ``torch``, not
the mmcv/mmengine/mmsegmentation stack (whose pins cap torch at 1.12 and
Python at 3.10).

Only inference is supported: the auxiliary FCN head used for training is
ignored, and BatchNorm always runs in eval mode.
"""
from __future__ import annotations

import os
import pickle
from typing import Optional

import cv2
import numpy as np

import torch
import torch.nn.functional as F
from torch import nn

# Preprocessing / palette from the training config (RGB order).
MEAN = (123.675, 116.28, 103.53)
STD = (58.395, 57.12, 57.375)
# Test-time Resize(scale=(2048, 512), keep_ratio=True) and the fixed 512x512
# minimum size the data preprocessor pads to.
RESIZE_SCALE = (2048, 512)
PAD_SIZE = (512, 512)
# class 0 = background, class 1 = water. Output PNGs encode water as RGB
# (128, 0, 0) on black, which is what downstream raster merging expects.
PALETTE = np.array([[0, 0, 0], [128, 0, 0]], dtype=np.uint8)


def _build_modules():
    """Define the network classes (deferred so torch stays optional)."""

    class ConvBN(nn.Sequential):
        """conv -> BN -> ReLU, named like mmcv's ConvModule (.conv/.bn)."""

        def __init__(self, cin, cout, k, padding=0, dilation=1, groups=1):
            super().__init__()
            self.conv = nn.Conv2d(cin, cout, k, padding=padding,
                                  dilation=dilation, groups=groups, bias=False)
            self.bn = nn.BatchNorm2d(cout)
            self.act = nn.ReLU(inplace=True)

    class DSConv(nn.Module):
        """Depthwise-separable ConvModule (.depthwise_conv/.pointwise_conv)."""

        def __init__(self, cin, cout, k=3, padding=1, dilation=1):
            super().__init__()
            self.depthwise_conv = ConvBN(cin, cin, k, padding, dilation, groups=cin)
            self.pointwise_conv = ConvBN(cin, cout, 1)

        def forward(self, x):
            return self.pointwise_conv(self.depthwise_conv(x))

    class Bottleneck(nn.Module):
        expansion = 4

        def __init__(self, cin, planes, stride, dilation, downsample):
            super().__init__()
            cout = planes * self.expansion
            self.conv1 = nn.Conv2d(cin, planes, 1, bias=False)
            self.bn1 = nn.BatchNorm2d(planes)
            # "pytorch" style: the stride lives on the 3x3 conv.
            self.conv2 = nn.Conv2d(planes, planes, 3, stride=stride,
                                   padding=dilation, dilation=dilation, bias=False)
            self.bn2 = nn.BatchNorm2d(planes)
            self.conv3 = nn.Conv2d(planes, cout, 1, bias=False)
            self.bn3 = nn.BatchNorm2d(cout)
            self.relu = nn.ReLU(inplace=True)
            self.downsample = None
            if downsample:
                self.downsample = nn.Sequential(
                    nn.Conv2d(cin, cout, 1, stride=stride, bias=False),
                    nn.BatchNorm2d(cout),
                )

        def forward(self, x):
            out = self.relu(self.bn1(self.conv1(x)))
            out = self.relu(self.bn2(self.conv2(out)))
            out = self.bn3(self.conv3(out))
            identity = x if self.downsample is None else self.downsample(x)
            return self.relu(out + identity)

    class ResNetV1c50(nn.Module):
        """ResNet-50 with a 3x3-conv deep stem, output stride 8
        (strides (1,2,1,1), dilations (1,1,2,4), contract_dilation=True)."""

        def __init__(self):
            super().__init__()
            self.stem = nn.Sequential(
                nn.Conv2d(3, 32, 3, stride=2, padding=1, bias=False),
                nn.BatchNorm2d(32), nn.ReLU(inplace=True),
                nn.Conv2d(32, 32, 3, padding=1, bias=False),
                nn.BatchNorm2d(32), nn.ReLU(inplace=True),
                nn.Conv2d(32, 64, 3, padding=1, bias=False),
                nn.BatchNorm2d(64), nn.ReLU(inplace=True),
            )
            self.maxpool = nn.MaxPool2d(3, stride=2, padding=1)
            cin = 64
            for i, (planes, blocks, stride, dil) in enumerate(
                    [(64, 3, 1, 1), (128, 4, 2, 1), (256, 6, 1, 2), (512, 3, 1, 4)], 1):
                layers = []
                for b in range(blocks):
                    # contract_dilation: first block of a dilated stage uses dil//2.
                    d = dil // 2 if (b == 0 and dil > 1) else dil
                    s = stride if b == 0 else 1
                    need_ds = b == 0 and (s != 1 or cin != planes * 4)
                    layers.append(Bottleneck(cin, planes, s, d, need_ds))
                    cin = planes * 4
                setattr(self, f"layer{i}", nn.Sequential(*layers))

        def forward(self, x):
            x = self.maxpool(self.stem(x))
            c1 = self.layer1(x)
            c2 = self.layer2(c1)
            c3 = self.layer3(c2)
            c4 = self.layer4(c3)
            return c1, c2, c3, c4

    class ImagePool(nn.Sequential):
        def __init__(self, cin, cout):
            super().__init__(nn.AdaptiveAvgPool2d(1), ConvBN(cin, cout, 1))

    class DeepLabV3PlusHead(nn.Module):
        def __init__(self, num_classes):
            super().__init__()
            dilations = (1, 12, 24, 36)
            self.image_pool = ImagePool(2048, 512)
            self.aspp_modules = nn.ModuleList(
                [ConvBN(2048, 512, 1)] +
                [DSConv(2048, 512, 3, padding=d, dilation=d) for d in dilations[1:]])
            self.bottleneck = ConvBN(5 * 512, 512, 3, padding=1)
            self.c1_bottleneck = ConvBN(256, 48, 1)
            self.sep_bottleneck = nn.Sequential(
                DSConv(512 + 48, 512), DSConv(512, 512))
            self.conv_seg = nn.Conv2d(512, num_classes, 1)

        def forward(self, c1, c4):
            pooled = F.interpolate(self.image_pool(c4), size=c4.shape[2:],
                                   mode="bilinear", align_corners=False)
            outs = [pooled] + [m(c4) for m in self.aspp_modules]
            x = self.bottleneck(torch.cat(outs, dim=1))
            low = self.c1_bottleneck(c1)
            x = F.interpolate(x, size=low.shape[2:], mode="bilinear",
                              align_corners=False)
            x = self.sep_bottleneck(torch.cat([x, low], dim=1))
            return self.conv_seg(x)

    class DeepLabV3Plus(nn.Module):
        def __init__(self, num_classes=2):
            super().__init__()
            self.backbone = ResNetV1c50()
            self.decode_head = DeepLabV3PlusHead(num_classes)

        def forward(self, x):
            c1, _, _, c4 = self.backbone(x)
            return self.decode_head(c1, c4)

    return DeepLabV3Plus


def _load_state_dict(path: str) -> dict:
    """Read an mmengine checkpoint's weights without needing mmengine.

    The checkpoint pickles mmengine bookkeeping objects (message hub, meta);
    unknown ``mm*`` classes are replaced with inert stand-ins so only the
    tensors in ``state_dict`` are materialised. Only load checkpoints you
    trust -- this still unpickles arbitrary non-``mm*`` globals.
    """
    class _StubMeta(type):
        def __getattr__(cls, name):
            if name.startswith("__"):
                raise AttributeError(name)
            return lambda *a, **k: cls()

    class _Stub(metaclass=_StubMeta):
        def __init__(self, *a, **k):
            pass

        def __setstate__(self, state):
            pass

    class _Unpickler(pickle.Unpickler):
        def find_class(self, module, name):
            if module.split(".")[0] in ("mmengine", "mmseg", "mmcv"):
                return type(name, (_Stub,), {})
            return super().find_class(module, name)

    class _PickleModule:
        Unpickler = _Unpickler
        load = staticmethod(pickle.load)
        loads = staticmethod(pickle.loads)

    ckpt = torch.load(path, map_location="cpu", weights_only=False,
                      pickle_module=_PickleModule)
    state = ckpt["state_dict"] if "state_dict" in ckpt else ckpt
    # Drop the training-only auxiliary head.
    return {k: v for k, v in state.items() if not k.startswith("auxiliary_head.")}


class Segmenter:
    """Loads the checkpoint once and segments images into a water mask."""

    def __init__(self, checkpoint_file: str, device: Optional[str] = None):
        self.device = torch.device(
            device or ("cuda" if torch.cuda.is_available() else "cpu"))
        self.model = _build_modules()(num_classes=2)
        self.model.load_state_dict(_load_state_dict(checkpoint_file))
        self.model.to(self.device).eval()

    @torch.no_grad()
    def predict(self, bgr: np.ndarray) -> np.ndarray:
        """Segment a BGR uint8 image; returns an HxW uint8 mask (1 = water)."""
        h, w = bgr.shape[:2]
        # mmcv imrescale: fit inside RESIZE_SCALE, preserving aspect ratio.
        scale = min(max(RESIZE_SCALE) / max(h, w), min(RESIZE_SCALE) / min(h, w))
        nw, nh = int(w * scale + 0.5), int(h * scale + 0.5)
        img = cv2.resize(bgr, (nw, nh), interpolation=cv2.INTER_LINEAR)

        rgb = img[:, :, ::-1].astype(np.float32)
        rgb = (rgb - np.array(MEAN, np.float32)) / np.array(STD, np.float32)
        x = torch.from_numpy(rgb.transpose(2, 0, 1).copy())[None].to(self.device)
        # Pad (bottom/right, with zeros) up to the minimum 512x512 input size.
        x = F.pad(x, (0, max(PAD_SIZE[1] - nw, 0), 0, max(PAD_SIZE[0] - nh, 0)))

        logits = self.model(x)
        logits = F.interpolate(logits, size=x.shape[2:], mode="bilinear",
                               align_corners=False)[:, :, :nh, :nw]
        logits = F.interpolate(logits, size=(h, w), mode="bilinear",
                               align_corners=False)
        return logits.argmax(dim=1)[0].to(torch.uint8).cpu().numpy()

    def segment_file(self, img_path: str, out_path: str) -> None:
        bgr = cv2.imread(img_path, cv2.IMREAD_COLOR)
        if bgr is None:
            raise ValueError(f"Could not read image: {img_path}")
        mask = self.predict(bgr)
        # PALETTE is RGB; cv2.imwrite expects BGR.
        cv2.imwrite(out_path, PALETTE[mask][:, :, ::-1])


def _files_with_suffix(directory: str, suffix: str) -> list[str]:
    return sorted(f for f in os.listdir(directory) if f.endswith(suffix))


def _subfolders(folder: str) -> list[str]:
    return [os.path.join(root, d)
            for root, dirs, _ in os.walk(folder) for d in dirs]


def process_img_folder(img_folder, output_folder, checkpoint_file,
                       img_suffix="png", device=None):
    """Segment every image in ``img_folder`` and write binary maps to
    ``output_folder``.

    ``img_folder`` can hold images directly, or be a folder of
    ``<year>/<site>`` subfolders of images (results mirror that layout under
    ``output_folder``). Do not mix images and subfolders in one folder.
    """
    os.makedirs(output_folder, exist_ok=True)
    segmenter = Segmenter(checkpoint_file, device)

    def run(src, dst):
        os.makedirs(dst, exist_ok=True)
        for name in _files_with_suffix(src, img_suffix):
            segmenter.segment_file(os.path.join(src, name),
                                   os.path.join(dst, name))

    if _files_with_suffix(img_folder, img_suffix):
        run(img_folder, output_folder)
        return
    for sub in _subfolders(img_folder):
        if _files_with_suffix(sub, img_suffix):
            parts = os.path.normpath(sub).split(os.sep)[-2:]
            run(sub, os.path.join(output_folder, *parts))


def process_single_img(img_file, out_file, checkpoint_file,
                       img_suffix="png", device=None):
    """Segment one image and write the binary map to ``out_file``."""
    os.makedirs(os.path.dirname(os.path.abspath(out_file)), exist_ok=True)
    Segmenter(checkpoint_file, device).segment_file(img_file, out_file)
