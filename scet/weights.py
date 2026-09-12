"""Fetch the pretrained segmentation checkpoint used by process_folder/process_image.

The checkpoint (~360MB) is too large for a PyPI sdist/wheel (PyPI caps
individual files at 100MB), so it's hosted as a GitHub Release asset instead
and downloaded on demand into a local cache directory.
"""
from __future__ import annotations

import os
import urllib.request

from tqdm import tqdm

RELEASE_TAG = "weights-v1"
WEIGHTS_FILENAME = "iter_100000.pth"
WEIGHTS_URL = (
    "https://github.com/lubyant/Shoreline-Change-Estimation-Toolkit/"
    f"releases/download/{RELEASE_TAG}/{WEIGHTS_FILENAME}"
)


def _default_cache_dir() -> str:
    cache_home = os.environ.get("XDG_CACHE_HOME") or os.path.join(
        os.path.expanduser("~"), ".cache"
    )
    return os.path.join(cache_home, "scet-toolkit")


def download_weights(dest_dir: str | None = None, force: bool = False) -> str:
    """Download the pretrained checkpoint, caching it locally.

    Parameters:
    - dest_dir: directory to save the checkpoint in (default: a per-user
      cache directory under XDG_CACHE_HOME or ~/.cache).
    - force: re-download even if the file already exists.

    Returns:
    - str: local path to the checkpoint file.
    """
    dest_dir = dest_dir or _default_cache_dir()
    os.makedirs(dest_dir, exist_ok=True)
    dest_path = os.path.join(dest_dir, WEIGHTS_FILENAME)

    if os.path.exists(dest_path) and not force:
        return dest_path

    tmp_path = dest_path + ".part"
    with urllib.request.urlopen(WEIGHTS_URL) as response:
        total = int(response.headers.get("Content-Length", 0))
        with open(tmp_path, "wb") as f, tqdm(
            total=total, unit="B", unit_scale=True, desc=WEIGHTS_FILENAME
        ) as bar:
            while chunk := response.read(1 << 20):
                f.write(chunk)
                bar.update(len(chunk))

    os.replace(tmp_path, dest_path)
    return dest_path
