"""End-to-end tests: synthetic rasters -> scet.pipeline -> OpenDSAS `dsas` CLI
-> shapefile outputs, plus the Fréchet metric over Image-extracted shorelines.

The pipeline tests need the real `dsas` executable and are skipped when it is
not installed (``pip install opendsas``).
"""
import geopandas as gpd
import pytest

from scet.frechet import frechet_change_rates
from scet.image import Image, merge_shorelines_from_images
from scet.pipeline import (
    generate_result_from_baseline,
    generate_result_from_folder,
    generate_result_from_image,
)
from tests.conftest import requires_dsas

OUTPUTS = ["shoreline.shp", "baseline.shp", "transect.shp", "intersection.shp"]


@requires_dsas
@pytest.mark.e2e
def test_generate_result_from_folder_produces_all_four_layers(image_folder, options, tmp_path):
    out = tmp_path / "result"
    generate_result_from_folder(image_folder, out, options)

    for name in OUTPUTS:
        assert (out / name).exists(), f"missing {name}"

    shoreline = gpd.read_file(out / "shoreline.shp")
    assert set(shoreline["year"]) == {2000, 2010, 2020}
    assert shoreline.crs.to_epsg() == 26917

    transect = gpd.read_file(out / "transect.shp")
    assert len(transect) > 0
    assert "ChangeRate" in transect.columns

    intersection = gpd.read_file(out / "intersection.shp")
    # one intersection per (transect, shoreline date)
    assert len(intersection) == pytest.approx(len(transect) * 3, abs=len(transect))
    assert {"BaselineId", "TransectId", "Date", "ref_dist"}.issubset(intersection.columns)


@requires_dsas
@pytest.mark.e2e
def test_prograding_shoreline_gives_a_consistently_signed_change_rate(image_folder, options, tmp_path):
    out = tmp_path / "result"
    generate_result_from_folder(image_folder, out, options)
    rates = gpd.read_file(out / "transect.shp")["ChangeRate"].dropna()
    # The boundary marches the same direction every epoch, so the sign of the
    # mean rate should dominate (not a 50/50 split of noise).
    assert len(rates) > 0
    frac_positive = (rates > 0).mean()
    assert frac_positive > 0.8 or frac_positive < 0.2


@requires_dsas
@pytest.mark.e2e
def test_generate_result_from_baseline_reuses_a_supplied_baseline(image_folder, options, tmp_path):
    # First derive a baseline the normal way, then feed it back in.
    first = tmp_path / "first"
    generate_result_from_folder(image_folder, first, options)

    out = tmp_path / "second"
    generate_result_from_baseline(image_folder, first / "baseline.shp", out, options)

    assert (out / "shoreline.shp").exists()
    assert (out / "transect.shp").exists()
    assert (out / "intersection.shp").exists()
    assert not (out / "baseline.shp").exists()  # not re-derived


def test_generate_result_from_folder_rejects_a_single_image(options, tmp_path):
    from tests.synthetic import synthetic_raster

    folder = tmp_path / "one"
    synthetic_raster(folder / "1_2000.tif")
    with pytest.raises(RuntimeError, match="Too few images"):
        generate_result_from_folder(folder, tmp_path / "out", options)


def test_generate_result_from_image_writes_only_a_shoreline(raster_factory, options, tmp_path):
    path = raster_factory("1_2004.tif")
    out = tmp_path / "single"
    shoreline_path = generate_result_from_image(path, out, options)

    assert shoreline_path == out / "shoreline.shp"
    assert shoreline_path.exists()
    assert not (out / "transect.shp").exists()
    gdf = gpd.read_file(shoreline_path)
    assert list(gdf["year"]) == [2004]


def test_frechet_change_rates_over_image_extracted_shorelines(image_folder, options):
    """The Fréchet port consumes scet.image.Shoreline objects directly."""
    images = [Image(p, options) for p in sorted(image_folder.iterdir())]
    shorelines = merge_shorelines_from_images(images, images[0].proj_wkt)

    rates = frechet_change_rates(shorelines)

    assert len(rates) == 2  # 2000->2010, 2010->2020
    assert [(r.year_start, r.year_end) for r in rates] == [(2000, 2010), (2010, 2020)]
    for r in rates:
        assert r.distance >= 0
        assert r.rate == pytest.approx(r.distance / 10.0)
