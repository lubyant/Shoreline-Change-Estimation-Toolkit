"""The raster helpers read bands via rasterio (no GDAL needed)."""
from __future__ import annotations

import numpy as np
import rasterio
from rasterio.transform import from_origin

from scet.extract_merge_data import (
    get_img_from_raster,
    get_img_source_data_from_raster,
    get_infrared_img_from_raster,
)


def _write_4band(path):
    data = np.stack([np.full((4, 5), v, np.uint8) for v in (10, 20, 30, 40)])
    with rasterio.open(
        path, "w", driver="GTiff", height=4, width=5, count=4, dtype="uint8",
        crs="EPSG:26917", transform=from_origin(0, 4, 1, 1),
    ) as dst:
        dst.write(data)


def test_raster_band_helpers(tmp_path):
    _write_4band(tmp_path / "r.tif")

    bgr = get_img_from_raster(str(tmp_path), "r.tif")
    assert bgr.shape == (4, 5, 3)
    assert bgr[0, 0].tolist() == [30, 20, 10]  # bands 1,2,3 -> R,G,B -> BGR

    b1, b2, b3 = get_img_source_data_from_raster(str(tmp_path), "r.tif")
    assert (b1[0, 0], b2[0, 0], b3[0, 0]) == (10, 20, 30)

    assert get_infrared_img_from_raster(str(tmp_path), "r.tif")[0, 0] == 40
