"""Shared fixtures for the scet test-suite. Raster helpers live in
``tests.synthetic`` so test modules can import the constants directly."""
from __future__ import annotations

import shutil

import pytest

from scet.options import Options
from tests.synthetic import synthetic_raster


@pytest.fixture
def raster_factory(tmp_path):
    """Callable ``(name, boundary_shift=0) -> Path`` writing a raster under tmp_path."""

    def _make(name: str, boundary_shift: int = 0):
        return synthetic_raster(tmp_path / name, boundary_shift)

    return _make


@pytest.fixture
def image_folder(tmp_path):
    """A folder of three dated rasters (2000, 2010, 2020), progressively prograded.

    Filenames end in a 4-digit year, which ``Image`` parses for its date.
    """
    folder = tmp_path / "images"
    folder.mkdir()
    for year, shift in [(2000, 0), (2010, 10), (2020, 20)]:
        synthetic_raster(folder / f"1_{year}.tif", boundary_shift=shift)
    return folder


@pytest.fixture
def options():
    """Small transect parameters so ``dsas`` runs fast in the e2e test."""
    opts = Options()
    opts.transect_length = 120
    opts.transect_spacing = 15
    return opts


requires_dsas = pytest.mark.skipif(
    shutil.which("dsas") is None,
    reason="the 'dsas' executable (pip install opendsas) is not on PATH",
)
