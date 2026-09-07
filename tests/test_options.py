"""Unit tests for scet.options."""
import pytest

from scet.options import IntersectionMode, Options, TransectOrientation


def test_defaults_match_the_cpp_engine():
    opts = Options()
    assert opts.smooth_factor == 1
    assert opts.transect_length == 500
    assert opts.transect_spacing == 30
    assert opts.edge_distance == 100
    assert opts.shoreline_least_factor == 0.5
    assert opts.intersection_mode is IntersectionMode.CLOSEST
    assert opts.transect_orient is TransectOrientation.MIX


def test_enum_values_are_the_cli_spellings():
    assert IntersectionMode.CLOSEST.value == "closest"
    assert IntersectionMode.FARTHEST.value == "farthest"
    assert {o.value for o in TransectOrientation} == {"left", "right", "mix"}


def test_from_json_reads_the_options_subkey_and_coerces_types():
    opts = Options.from_json(
        {
            "options": {
                "smooth_factor": "3",
                "edge_distance": "50",
                "shoreline_least_factor": "0.25",
                "transect_length": "250",
                "transect_spacing": "10",
                "intersection_mode": "farthest",
                "transect_orientation": "left",
            }
        }
    )
    assert opts.smooth_factor == 3 and isinstance(opts.smooth_factor, int)
    assert opts.edge_distance == 50
    assert opts.shoreline_least_factor == 0.25
    assert opts.transect_length == 250.0 and isinstance(opts.transect_length, float)
    assert opts.transect_spacing == 10.0
    assert opts.intersection_mode is IntersectionMode.FARTHEST
    assert opts.transect_orient is TransectOrientation.LEFT


def test_from_json_accepts_a_bare_dict_without_the_options_key():
    opts = Options.from_json({"transect_length": 42})
    assert opts.transect_length == 42


def test_from_json_ignores_unknown_keys_and_keeps_defaults():
    opts = Options.from_json({"options": {"nonsense": 1}})
    assert opts.transect_length == Options().transect_length


def test_from_json_rejects_an_invalid_enum_spelling():
    with pytest.raises(ValueError):
        Options.from_json({"options": {"intersection_mode": "sideways"}})
