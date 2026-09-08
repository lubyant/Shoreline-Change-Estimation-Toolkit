"""Ported from the old C++ Boost tests (utility.test.cpp: TestFrechetDistance,
TestModifiedFrechetDistance) plus a couple of checks for the shoreline-level
helpers."""
import math

import numpy as np
import pytest

from scet.frechet import (
    discrete_frechet,
    flag_outliers,
    frechet_change_rates,
    modified_frechet,
    orientation_free_frechet,
    resample_polyline,
)

SQRT2 = math.sqrt(2)


@pytest.mark.parametrize(
    "line1, line2, expected",
    [
        ([(0, 0), (1, 1), (2, 2)], [(0, 0), (1, 1), (2, 2)], 0.0),
        ([(0, 0), (1, 1), (2, 2)], [(1, 1), (2, 2), (3, 3)], SQRT2),
        ([(0, 0), (1, 1), (2, 2)], [(0, 0), (1, 1)], SQRT2),
        ([(0, 0), (1, 0), (2, 0)], [(0, 1), (1, 1), (2, 1)], 1.0),
        ([(0, 1), (1, 2), (2, 1), (3, 3)], [(0, 0), (1, 0), (2, 0), (3, 0)], 3.0),
    ],
)
def test_discrete_frechet(line1, line2, expected):
    assert discrete_frechet(line1, line2) == pytest.approx(expected)


@pytest.mark.parametrize(
    "line1, line2, expected",
    [
        ([(0, 0), (1, 1), (2, 2)], [(0, 0), (1, 1), (2, 2)], 0.0),
        ([(0, 0), (1, 1), (2, 2)], [(1, 1), (2, 2), (3, 3)], 0.0),
        ([(0, 0), (1, 1), (2, 2)], [(2, 0), (1, 1), (0, 2)], 2.0),
    ],
)
def test_modified_frechet(line1, line2, expected):
    assert modified_frechet(line1, line2) == pytest.approx(expected)


def test_resample_polyline_even_spacing():
    out = resample_polyline([(0, 0), (10, 0)], 5)
    assert np.allclose(out[:, 0], [0, 2.5, 5, 7.5, 10])
    assert np.allclose(out[:, 1], 0)


def test_resample_polyline_preserves_endpoints_and_count():
    line = [(0, 0), (1, 3), (4, 4), (9, 1)]
    out = resample_polyline(line, 7)
    assert len(out) == 7
    assert np.allclose(out[0], line[0])
    assert np.allclose(out[-1], line[-1])


def test_orientation_free_frechet_ignores_vertex_order():
    a = [(0, 0), (1, 0), (2, 0), (3, 0)]
    b = [(3, 1), (2, 1), (1, 1), (0, 1)]  # same shape, opposite direction
    assert orientation_free_frechet(a, b) == pytest.approx(0.0)


def test_frechet_change_rates_divides_by_year_gap():
    base = [(x, 0.0) for x in range(11)]
    # 2000: flat. 2004: a single bump so the gap along the line varies by 5.
    bumped = [(x, 5.0 if x == 5 else 0.0) for x in range(11)]
    rates = frechet_change_rates(
        [base, bumped], dates=[2004, 2000]
    )  # deliberately out of order
    assert len(rates) == 1
    (r,) = rates
    assert (r.year_start, r.year_end) == (2000, 2004)
    assert r.distance == pytest.approx(5.0)
    assert r.rate == pytest.approx(5.0 / 4)


def test_flag_outliers_one_sided_vs_two_sided():
    vals = [1.0, 1.1, 0.9, 1.05, 0.95, 10.0]
    assert flag_outliers(vals, outlier_rate=2.0) == [
        False,
        False,
        False,
        False,
        False,
        True,
    ]
    # A low outlier is only caught in two-sided mode.
    low = [10.0, 10.1, 9.9, 10.05, 9.95, 1.0]
    assert flag_outliers(low, outlier_rate=2.0) == [False] * 6
    assert flag_outliers(low, outlier_rate=2.0, two_sided=True)[-1] is True


def test_flag_outliers_degenerate_inputs():
    assert flag_outliers([]) == []
    assert flag_outliers([5.0]) == [False]
    assert flag_outliers([3.0, 3.0, 3.0]) == [False, False, False]
