#!/bin/bash
conda install pytorch==1.12.1 torchvision==0.13.1 torchaudio==0.12.1 cudatoolkit=11.3 -c pytorch -y
conda install boost gdal opencv pybind11 compilers pillow rasterio pyproj tqdm ftfy regex cmake make -c conda-forge -y
pip install -U openmim
mim install mmengine
mim install "mmcv==2.0.0rc4"
pip install "mmsegmentation>=1.0.0"
python setup.py build
python setup.py install
