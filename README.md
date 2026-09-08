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

`torch`/`torchvision`/`torchaudio` are CUDA-build-sensitive; if the plain pip
install doesn't pick the right wheel for your platform, install them from
https://pytorch.org first, matching your CUDA version, then re-run. `mmcv-lite`
(no compiled CUDA ops, unlike `mmcv`) is used instead of `mmcv` since this
project's segmentation config doesn't need them and `mmcv-lite` installs via
plain pip with no CUDA toolchain required.

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
bash build.sh  # pip install -e ".[dl]"
```

### Testing

```bash
pip install -e ".[test]"
pytest                 # full suite, including end-to-end pipeline tests
pytest -m "not e2e"    # unit tests only (skip anything that shells out to `dsas`)
```

The end-to-end tests build synthetic georeferenced rasters, run the full
`images -> shoreline/baseline -> dsas cast/cal` pipeline, and check the
output shapefiles. They need the `dsas` executable (installed with the
`opendsas` dependency) on `PATH` and self-skip if it is missing.
