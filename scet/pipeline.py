"""End-to-end shoreline-change pipeline: images -> shoreline/baseline vector
files -> OpenDSAS CLI (transects, intersections, change rates).

Replaces shorelinecalculator.cpp's controller()/dsas()/
digital_shoreline_analysis_system() and the pybind bindings in pybind.cpp.
"""
from __future__ import annotations

from pathlib import Path
from typing import List

from . import dsas_cli
from .image import Image, merge_baseline_from_images, merge_shorelines_from_images
from .options import Options
from .shapefile_io import save_baseline, save_shorelines


def _collect_image_paths(folder: Path) -> List[Path]:
    return sorted(p for p in folder.rglob("*") if p.is_file())


def generate_result_from_folder(
    input_folder: Path | str,
    output_folder: Path | str,
    options: Options,
    bid_field: str = "Id",
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> None:
    """Extract shorelines from every image in `input_folder`, derive a
    baseline from the earliest non-overlapping shoreline geometry, then
    delegate transect casting and change-rate calculation to the `dsas` CLI.

    Writes shoreline.shp, baseline.shp, transect.shp, and intersection.shp
    into `output_folder`.
    """
    input_folder = Path(input_folder)
    output_folder = Path(output_folder)
    output_folder.mkdir(parents=True, exist_ok=True)

    image_paths = _collect_image_paths(input_folder)
    if len(image_paths) < 2:
        raise RuntimeError(f"Too few images to process in {input_folder}")

    images = [Image(path, options) for path in image_paths]
    template_wkt = images[0].proj_wkt

    shorelines = merge_shorelines_from_images(images, template_wkt)
    baseline_shorelines = merge_baseline_from_images(images)

    shoreline_path = output_folder / "shoreline.shp"
    baseline_path = output_folder / "baseline.shp"
    transect_path = output_folder / "transect.shp"
    intersect_path = output_folder / "intersection.shp"

    save_shorelines(shorelines, template_wkt, shoreline_path, date_field, date_format)
    save_baseline(baseline_shorelines, template_wkt, baseline_path, bid_field)

    dsas_cli.cast(baseline_path, transect_path, options, bid_field=bid_field)
    dsas_cli.cal(
        transect_path,
        shoreline_path,
        intersect_path,
        options,
        date_field=date_field,
        date_format=date_format,
    )


def generate_result_from_image(
    image_path: Path | str,
    output_folder: Path | str,
    options: Options,
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> Path:
    """Extract a shoreline from a single image and save it. A single image
    has no baseline/history to compare against, so no transects or
    intersections are produced -- mirrors the old generate_result_from_image
    pybind function."""
    output_folder = Path(output_folder)
    output_folder.mkdir(parents=True, exist_ok=True)

    image = Image(image_path, options)
    shorelines = merge_shorelines_from_images([image])

    shoreline_path = output_folder / "shoreline.shp"
    save_shorelines(shorelines, image.proj_wkt, shoreline_path, date_field, date_format)
    return shoreline_path


def generate_result_from_baseline(
    image_folder: Path | str,
    baseline_shp_path: Path | str,
    output_folder: Path | str,
    options: Options,
    bid_field: str = "Id",
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> None:
    """Same as generate_result_from_folder, but uses a user-supplied baseline
    instead of deriving one from the imagery."""
    output_folder = Path(output_folder)
    output_folder.mkdir(parents=True, exist_ok=True)

    image_paths = _collect_image_paths(Path(image_folder))
    if len(image_paths) < 2:
        raise RuntimeError(f"Too few images to process in {image_folder}")

    images = [Image(path, options) for path in image_paths]
    template_wkt = images[0].proj_wkt
    shorelines = merge_shorelines_from_images(images, template_wkt)

    shoreline_path = output_folder / "shoreline.shp"
    transect_path = output_folder / "transect.shp"
    intersect_path = output_folder / "intersection.shp"

    save_shorelines(shorelines, template_wkt, shoreline_path, date_field, date_format)

    dsas_cli.cast(baseline_shp_path, transect_path, options, bid_field=bid_field)
    dsas_cli.cal(
        transect_path,
        shoreline_path,
        intersect_path,
        options,
        date_field=date_field,
        date_format=date_format,
    )
