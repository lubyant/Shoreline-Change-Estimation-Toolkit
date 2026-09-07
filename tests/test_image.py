"""Unit tests for scet.image: geometry helpers plus the Image extraction
pipeline run against synthetic georeferenced rasters."""
import datetime as dt

import pytest

from scet.image import (
    GeoInfo,
    Image,
    Point,
    Shoreline,
    merge_baseline_from_images,
    merge_shorelines_from_images,
)
from tests.synthetic import ORIGIN_X, ORIGIN_Y, RASTER_SIZE


# -- pure geometry helpers ------------------------------------------------


def _geoinfo(x0, y0, x1, y1):
    """GeoInfo for an axis-aligned box with corners (x0, y1) top-left .. (x1, y0)."""
    gi = GeoInfo()
    gi.up_left = Point(x0, y1)
    gi.up_right = Point(x1, y1)
    gi.bottom_left = Point(x0, y0)
    gi.bottom_right = Point(x1, y0)
    return gi


def test_shoreline_len_counts_vertices():
    s = Shoreline(0, 2000, 1, GeoInfo(), dt.date(2000, 1, 1), [Point(0, 0), Point(1, 1)])
    assert len(s) == 2


def test_geoinfo_is_overlaid_detects_overlap_and_disjointness():
    a = _geoinfo(0, 0, 10, 10)
    assert a.is_overlaid(_geoinfo(5, 5, 15, 15)) is True
    assert a.is_overlaid(_geoinfo(20, 20, 30, 30)) is False


def test_geoinfo_contains_point():
    gi = _geoinfo(0, 0, 10, 10)
    assert gi.contains_point(Point(5, 5)) is True
    assert gi.contains_point(Point(-1, 5)) is False
    assert gi.contains_point(Point(5, 11)) is False


# -- Image extraction ---------------------------------------------------


def test_image_parses_year_and_id_from_filename(raster_factory):
    path = raster_factory("7_2003.tif")
    img = Image(path, options=_opts())
    assert img.year == 2003
    assert img.date == dt.date(2003, 1, 1)
    assert img.image_id == 7


def test_image_rejects_a_filename_without_a_year(raster_factory, tmp_path):
    src = raster_factory("1_2000.tif")
    bad = tmp_path / "no_year.tif"
    bad.write_bytes(src.read_bytes())
    with pytest.raises(ValueError):
        Image(bad, options=_opts())


def test_image_extracts_a_georeferenced_shoreline(raster_factory):
    img = Image(raster_factory("1_2000.tif"), options=_opts())
    assert img.proj_wkt  # CRS was read
    assert len(img.shorelines) >= 1

    shoreline = img.shorelines[0]
    assert len(shoreline) >= 2
    # Vertices were transformed from pixel space into the raster CRS: the
    # boundary sits near column 200, i.e. geo-x near ORIGIN_X + 200.
    xs = [p.x for p in shoreline.vertices]
    ys = [p.y for p in shoreline.vertices]
    assert ORIGIN_X + 150 < min(xs) and max(xs) < ORIGIN_X + 260
    assert all(ORIGIN_Y - RASTER_SIZE <= y <= ORIGIN_Y for y in ys)


def test_image_geoinfo_extent_matches_the_raster(raster_factory):
    img = Image(raster_factory("1_2000.tif"), options=_opts())
    gi = img.geo_info
    assert gi.up_left.x == pytest.approx(ORIGIN_X)
    assert gi.up_left.y == pytest.approx(ORIGIN_Y)
    assert gi.bottom_right.x == pytest.approx(ORIGIN_X + RASTER_SIZE)
    assert gi.bottom_right.y == pytest.approx(ORIGIN_Y - RASTER_SIZE)


def test_reproject_is_a_noop_when_target_crs_equals_source(raster_factory):
    img = Image(raster_factory("1_2000.tif"), options=_opts())
    before = [(p.x, p.y) for p in img.shorelines[0].vertices]
    img.reproject(img.proj_wkt)
    after = [(p.x, p.y) for p in img.shorelines[0].vertices]
    assert before == after


def test_reproject_to_wgs84_moves_vertices_into_lon_lat_range(raster_factory):
    img = Image(raster_factory("1_2000.tif"), options=_opts())
    projected = [(p.x, p.y) for p in img.shorelines[0].vertices]
    img.reproject("EPSG:4326")

    reprojected = [(p.x, p.y) for p in img.shorelines[0].vertices]
    assert reprojected != projected
    for x, y in reprojected:
        assert -180 <= x <= 180 and -90 <= y <= 90
    assert img.proj_wkt == "EPSG:4326"
    # geo_info corners were reprojected too
    assert -180 <= img.geo_info.up_left.x <= 180


# -- merging across images -------------------------------------------


def test_merge_shorelines_concatenates_every_images_shorelines(image_folder):
    imgs = [Image(p, _opts()) for p in sorted(image_folder.iterdir())]
    merged = merge_shorelines_from_images(imgs, imgs[0].proj_wkt)
    assert len(merged) == sum(len(i.shorelines) for i in imgs)
    assert {s.year for s in merged} == {2000, 2010, 2020}


def test_merge_baseline_dedupes_overlapping_geometry(image_folder):
    imgs = [Image(p, _opts()) for p in sorted(image_folder.iterdir())]
    merge_shorelines_from_images(imgs, imgs[0].proj_wkt)  # reproject onto common CRS
    baseline = merge_baseline_from_images(imgs)
    # All three rasters share the same footprint, so only the first image
    # contributes baseline geometry; the rest are fully contained.
    assert len(baseline) >= 1
    assert [b.shoreline_id for b in baseline] == list(range(len(baseline)))


def test_merge_shorelines_of_empty_list_is_empty():
    assert merge_shorelines_from_images([]) == []


def _opts():
    from scet.options import Options

    return Options()
