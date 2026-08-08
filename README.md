# SCET (Shoreline Change Estimation Toolkit)

A pure-Python toolkit for extracting shorelines from segmented raster imagery
and computing shoreline change rates. Transect casting, shoreline/transect
intersection, and change-rate calculation are delegated to the
[OpenDSAS](https://github.com/lubyant/OpenDSAS) CLI (`dsas`), a fast,
cross-platform reimplementation of USGS DSAS.

## Installation

```bash
pip install scet-toolkit
```

This installs `scet-toolkit`'s dependencies including `opendsas`, which
provides the `dsas` CLI used for transect/intersection/rate calculations.

For the optional deep-learning-based segmentation step (`scet.simple_mmseg`,
used by `SCET.method1`/`method2`), install the `dl` extra:

```bash
pip install "scet-toolkit[dl]"
```

`mmcv` is version- and CUDA-build-sensitive; if the plain pip install doesn't
pick the right wheel for your platform, run `mim install mmcv==2.0.0rc4`
afterward (see `build.sh`).

## Usage

```python
from scet import SCET, Config

config = Config(checkpoint_file="path/to/checkpoint.pth")
scet = SCET(config)
scet.method1(input_naip_zipfiles_folder="...", output_folder="...")
```

Or drive the shoreline pipeline directly, given a folder of already-segmented,
dated raster images (filenames ending in a 4-digit year, e.g. `1_2001.tif`):

```python
from scet.options import Options
from scet.pipeline import generate_result_from_folder

generate_result_from_folder("images/", "output/", Options())
```

This writes `shoreline.shp`, `baseline.shp`, `transect.shp`, and
`intersection.shp` to the output folder.

## Development

```bash
conda create -n my_env python==3.9
conda activate my_env
bash build.sh  # pip install -e ".[dl]" + mim install for mmcv
```
