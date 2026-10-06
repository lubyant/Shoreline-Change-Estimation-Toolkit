"""Tests for the pure-PyTorch DeepLabV3+ segmenter (scet.segmentation)."""
from __future__ import annotations

import cv2
import numpy as np
import pytest
import torch

from scet import segmentation


@pytest.fixture(scope="module")
def checkpoint(tmp_path_factory):
    """A randomly initialised checkpoint shaped like an mmengine one
    (``state_dict`` plus an auxiliary head that must be ignored)."""
    torch.manual_seed(0)
    model = segmentation._build_modules()(num_classes=2)
    state = dict(model.state_dict())
    state["auxiliary_head.conv_seg.weight"] = torch.zeros(2, 256, 1, 1)
    path = tmp_path_factory.mktemp("ckpt") / "weights.pth"
    torch.save({"state_dict": state, "meta": {}}, path)
    return str(path)


def test_state_dict_names_match_mmseg_layout():
    keys = set(segmentation._build_modules()().state_dict())
    for expected in (
        "backbone.stem.0.weight",
        "backbone.layer1.0.downsample.0.weight",
        "backbone.layer4.2.conv3.weight",
        "decode_head.image_pool.1.conv.weight",
        "decode_head.aspp_modules.0.conv.weight",
        "decode_head.aspp_modules.3.depthwise_conv.conv.weight",
        "decode_head.sep_bottleneck.1.pointwise_conv.bn.running_var",
        "decode_head.c1_bottleneck.conv.weight",
        "decode_head.conv_seg.bias",
    ):
        assert expected in keys


@pytest.mark.parametrize("shape", [(300, 200), (1000, 1000), (256, 1500)])
def test_predict_returns_mask_at_input_resolution(checkpoint, shape):
    seg = segmentation.Segmenter(checkpoint, device="cpu")
    img = np.random.default_rng(0).integers(0, 255, (*shape, 3), dtype=np.uint8)
    mask = seg.predict(img)
    assert mask.shape == shape
    assert mask.dtype == np.uint8
    assert set(np.unique(mask)) <= {0, 1}


def test_folder_processing_mirrors_layout_and_encodes_palette(checkpoint, tmp_path):
    src = tmp_path / "in" / "2020" / "123"
    src.mkdir(parents=True)
    img = np.random.default_rng(1).integers(0, 255, (128, 128, 3), dtype=np.uint8)
    cv2.imwrite(str(src / "0_0_a.png"), img)

    out = tmp_path / "out"
    segmentation.process_img_folder(
        str(tmp_path / "in"), str(out), checkpoint, device="cpu")

    result = cv2.imread(str(out / "2020" / "123" / "0_0_a.png"))
    assert result.shape == img.shape
    # Water is RGB (128, 0, 0) == BGR (0, 0, 128); everything else is black.
    colours = {tuple(c) for c in result.reshape(-1, 3)}
    assert colours <= {(0, 0, 0), (0, 0, 128)}


def test_single_image(checkpoint, tmp_path):
    img = np.zeros((64, 64, 3), np.uint8)
    cv2.imwrite(str(tmp_path / "a.png"), img)
    out = tmp_path / "sub" / "out.png"
    segmentation.process_single_img(
        str(tmp_path / "a.png"), str(out), checkpoint, device="cpu")
    assert out.exists()
