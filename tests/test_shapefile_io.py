"""Unit tests for scet.shapefile_io."""
import datetime as dt

import geopandas as gpd
import pytest
from shapely.geometry import LineString

from scet.image import GeoInfo, Point, Shoreline
from scet.shapefile_io import save_baseline, save_shorelines
from tests.synthetic import TEST_EPSG

# A minimal projected-CRS WKT (UTM 17N) so save_* can attach a CRS.
from pyproj import CRS

WKT = CRS.from_epsg(TEST_EPSG).to_wkt()


def _shoreline(year, xs, sid=0):
    return Shoreline(
        shoreline_id=sid,
        year=year,
        image_id=1,
        geo_info=GeoInfo(),
        date=dt.date(year, 1, 1),
        vertices=[Point(x, float(x)) for x in xs],
    )


def test_save_shorelines_writes_readable_features_with_a_date_field(tmp_path):
    out = tmp_path / "shoreline.shp"
    save_shorelines([_shoreline(2001, [0, 1, 2]), _shoreline(2005, [0, 1, 2, 3])], WKT, out)

    gdf = gpd.read_file(out)
    assert len(gdf) == 2
    assert gdf.crs.to_epsg() == TEST_EPSG
    assert list(gdf["Date"]) == ["2001/01/01", "2005/01/01"]
    assert set(gdf["year"]) == {2001, 2005}
    assert all(isinstance(g, LineString) for g in gdf.geometry)


def test_save_shorelines_honours_a_custom_date_field_and_format(tmp_path):
    out = tmp_path / "shoreline.shp"
    save_shorelines(
        [_shoreline(2001, [0, 1, 2])], WKT, out, date_field="SDate", date_format="%Y-%m-%d"
    )
    gdf = gpd.read_file(out)
    assert list(gdf["SDate"]) == ["2001-01-01"]


def test_save_shorelines_skips_segments_with_fewer_than_two_vertices(tmp_path):
    out = tmp_path / "shoreline.shp"
    save_shorelines([_shoreline(2001, [0]), _shoreline(2002, [0, 1, 2])], WKT, out)
    gdf = gpd.read_file(out)
    assert list(gdf["year"]) == [2002]


def test_save_shorelines_raises_when_nothing_has_two_vertices(tmp_path):
    with pytest.raises(ValueError):
        save_shorelines([_shoreline(2001, [0])], WKT, tmp_path / "shoreline.shp")


def test_save_baseline_writes_an_id_field_per_segment(tmp_path):
    out = tmp_path / "baseline.shp"
    save_baseline([_shoreline(2000, [0, 1, 2], sid=3), _shoreline(2000, [3, 4, 5], sid=7)], WKT, out)
    gdf = gpd.read_file(out)
    assert sorted(gdf["Id"]) == [3, 7]
    assert gdf.crs.to_epsg() == TEST_EPSG


def test_save_baseline_honours_a_custom_id_field(tmp_path):
    out = tmp_path / "baseline.shp"
    save_baseline([_shoreline(2000, [0, 1, 2], sid=1)], WKT, out, id_field="BID")
    gdf = gpd.read_file(out)
    assert "BID" in gdf.columns


def test_save_baseline_creates_missing_parent_directories(tmp_path):
    out = tmp_path / "nested" / "sub" / "baseline.shp"
    save_baseline([_shoreline(2000, [0, 1, 2], sid=1)], WKT, out)
    assert out.exists()
