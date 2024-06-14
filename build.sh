#!/bin/bash
mim install mmengine
mim install "mmcv>=2.0.0"
python setup.py build
python setup.py install