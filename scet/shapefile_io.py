"""Read/write shoreline & baseline vector files (.shp or .geojson).

Replaces the GDAL/OGR C++ calls in utility.cpp (save_lines, save_points,
get_tiff_proj, get_shp_proj) with geopandas, which the rest of the project
already depends on (see water_level_calibration.py).
"""
from __future__ import annotations

from pathlib import Path
from typing import Sequence

import geopandas as gpd
from pyproj import CRS
from shapely.geometry import LineString

from .image import Shoreline


def save_shorelines(
    shorelines: Sequence[Shoreline],
    proj_wkt: str,
    output_path: Path | str,
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> None:
    """Save shoreline polylines, one feature per shoreline segment, with a
    date field OpenDSAS's `dsas cal` can consume via --date-field/--date-format."""
    records = []
    geoms = []
    for shoreline in shorelines:
        if len(shoreline.vertices) < 2:
            continue
        geoms.append(LineString([(p.x, p.y) for p in shoreline.vertices]))
        records.append(
            {
                date_field: shoreline.date.strftime(date_format),
                "year": shoreline.year,
                "ImageId": shoreline.image_id,
            }
        )

    if not geoms:
        raise ValueError("No shoreline has at least 2 vertices; nothing to save.")

    gdf = gpd.GeoDataFrame(records, geometry=geoms, crs=CRS.from_wkt(proj_wkt))
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    gdf.to_file(output_path)


def save_baseline(
    baseline_shorelines: Sequence[Shoreline],
    proj_wkt: str,
    output_path: Path | str,
    id_field: str = "Id",
) -> None:
    """Save baseline polylines, one feature per contiguous baseline segment."""
    records = []
    geoms = []
    for shoreline in baseline_shorelines:
        if len(shoreline.vertices) < 2:
            continue
        geoms.append(LineString([(p.x, p.y) for p in shoreline.vertices]))
        records.append({id_field: shoreline.shoreline_id})

    if not geoms:
        raise ValueError("No baseline segment has at least 2 vertices; nothing to save.")

    gdf = gpd.GeoDataFrame(records, geometry=geoms, crs=CRS.from_wkt(proj_wkt))
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    gdf.to_file(output_path)
