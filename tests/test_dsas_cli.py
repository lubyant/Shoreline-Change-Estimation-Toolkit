"""Unit tests for scet.dsas_cli -- argv construction and executable lookup.

The actual `dsas` invocation is exercised by test_e2e_pipeline.py; here
subprocess.run is stubbed so we can assert on the command line.
"""
import pytest

from scet import dsas_cli
from scet.options import IntersectionMode, Options, TransectOrientation


@pytest.fixture
def captured_cmd(monkeypatch):
    """Stub subprocess.run + find_executable; return a list that receives argv."""
    calls = []
    monkeypatch.setattr(dsas_cli.subprocess, "run", lambda cmd, **kw: calls.append((cmd, kw)))
    monkeypatch.setattr(dsas_cli, "find_executable", lambda: "/usr/bin/dsas")
    return calls


def _pairs(argv):
    """Map every ``--flag value`` pair in argv to a dict."""
    return {argv[i]: argv[i + 1] for i in range(len(argv)) if argv[i].startswith("--")}


def test_find_executable_returns_path_when_present(monkeypatch):
    monkeypatch.setattr(dsas_cli.shutil, "which", lambda name: "/opt/bin/dsas")
    assert dsas_cli.find_executable() == "/opt/bin/dsas"


def test_find_executable_raises_a_helpful_error_when_missing(monkeypatch):
    monkeypatch.setattr(dsas_cli.shutil, "which", lambda name: None)
    with pytest.raises(RuntimeError, match="pip install opendsas"):
        dsas_cli.find_executable()


def test_cast_builds_the_expected_command(captured_cmd):
    opts = Options()
    opts.transect_length = 250
    opts.transect_spacing = 12
    opts.intersection_mode = IntersectionMode.FARTHEST
    opts.transect_orient = TransectOrientation.LEFT

    dsas_cli.cast("base.shp", "trans.shp", opts, bid_field="BID")

    (argv, kwargs), = captured_cmd
    assert argv[:2] == ["/usr/bin/dsas", "cast"]
    assert kwargs == {"check": True}
    pairs = _pairs(argv)
    assert pairs["--baseline"] == "base.shp"
    assert pairs["--output-transect"] == "trans.shp"
    assert pairs["--bid-field"] == "BID"
    assert pairs["--transect-length"] == "250"
    assert pairs["--transect-spacing"] == "12"
    assert pairs["--intersection-mode"] == "farthest"
    assert pairs["--transect-orientation"] == "left"


def test_cal_builds_the_expected_command(captured_cmd):
    dsas_cli.cal(
        "trans.shp", "shore.shp", "out.shp", Options(),
        date_field="SDate", date_format="%Y-%m-%d",
    )
    (argv, _), = captured_cmd
    assert argv[:2] == ["/usr/bin/dsas", "cal"]
    pairs = _pairs(argv)
    assert pairs["--transect"] == "trans.shp"
    assert pairs["--shoreline"] == "shore.shp"
    assert pairs["--output-intersect"] == "out.shp"
    assert pairs["--date-field"] == "SDate"
    assert pairs["--date-format"] == "%Y-%m-%d"


def test_run_builds_a_single_root_command_without_a_subcommand(captured_cmd):
    dsas_cli.run("base.shp", "shore.shp", "trans.shp", "int.shp", Options())
    (argv, _), = captured_cmd
    assert argv[0] == "/usr/bin/dsas"
    assert "cast" not in argv and "cal" not in argv
    pairs = _pairs(argv)
    assert pairs["--baseline"] == "base.shp"
    assert pairs["--shoreline"] == "shore.shp"
    assert pairs["--output-transect"] == "trans.shp"
    assert pairs["--output-intersect"] == "int.shp"


def test_path_objects_are_stringified(captured_cmd, tmp_path):
    dsas_cli.cast(tmp_path / "b.shp", tmp_path / "t.shp", Options())
    (argv, _), = captured_cmd
    assert all(isinstance(a, str) for a in argv)


def test_cast_propagates_a_nonzero_dsas_exit(monkeypatch):
    import subprocess

    monkeypatch.setattr(dsas_cli, "find_executable", lambda: "/usr/bin/dsas")

    def boom(cmd, **kw):
        raise subprocess.CalledProcessError(2, cmd)

    monkeypatch.setattr(dsas_cli.subprocess, "run", boom)
    with pytest.raises(subprocess.CalledProcessError):
        dsas_cli.cast("b.shp", "t.shp", Options())
