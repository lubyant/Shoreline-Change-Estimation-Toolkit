"""Synthetic georeferenced rasters for the scet test-suite.

``scet.image.Image`` expects a georeferenced binary land/water raster:
non-zero pixels are land, and the land/water boundary is the shoreline.
``synthetic_raster`` builds one with a wavy vertical boundary so
``cv2.findContours`` keeps enough vertices along it to survive the
edge-splitting and length-filtering inside ``Image``.
"""
from __future__ import annotations

from pathlib import Path

import numpy as np
import rasterio
from rasterio.transform import from_origin

# A projected CRS in metres (UTM 17N) -- transect lengths/spacings are metres.
TEST_EPSG = 26917
RASTER_SIZE = 400
# Origin chosen so shoreline coordinates land well away from zero.
ORIGIN_X = 1000.0
ORIGIN_Y = 5000.0


def synthetic_raster(
    path: Path, boundary_shift: int = 0, size: int = RASTER_SIZE
) -> Path:
    """Write a ``size``x``size`` binary land/water GeoTIFF to ``path``.

    Land (255) fills the left of a sinusoidal vertical boundary whose mean
    column is ``size // 2 + boundary_shift``; water (0) fills the right.
    Increasing ``boundary_shift`` between years simulates progradation.
    """
    arr = np.zeros((size, size), dtype=np.uint8)
    rows = np.arange(size)
    boundary = size // 2 + boundary_shift + (30 * np.sin(rows / 40.0)).astype(int)
    for y, bx in zip(rows, boundary):
        arr[y, : int(bx)] = 255

    transform = from_origin(ORIGIN_X, ORIGIN_Y, 1.0, 1.0)
    path.parent.mkdir(parents=True, exist_ok=True)
    with rasterio.open(
        path,
        "w",
        driver="GTiff",
        height=size,
        width=size,
        count=1,
        dtype="uint8",
        crs=f"EPSG:{TEST_EPSG}",
        transform=transform,
    ) as ds:
        ds.write(arr, 1)
    return path
