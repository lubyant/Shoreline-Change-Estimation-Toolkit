#!/bin/bash
set -e
# All dependencies (including the optional DL segmentation stack) are now
# declared in pyproject.toml. `mim install` is used for mmcv instead of plain
# pip since it selects the wheel matching your installed torch/CUDA build.
pip install -e ".[dl]"
mim install mmengine
mim install "mmcv==2.0.0rc4"
