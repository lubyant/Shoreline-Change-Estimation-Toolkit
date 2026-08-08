#!/bin/bash
set -e
# All dependencies (including the optional DL segmentation stack) are
# declared in pyproject.toml -- no mim/conda steps needed.
pip install -e ".[dl]"
