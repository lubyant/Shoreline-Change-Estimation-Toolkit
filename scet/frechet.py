"""Fréchet-distance shoreline-change metrics.

Pure-Python port of the Fréchet code that used to live in the C++ engine
(``utility.cpp``: ``frechet_distance``, ``equal_divided_polyline``,
``modified_frechet_distance``; ``shorelinecalculator.cpp``: the per-image
outlier pass in ``dsas::frechet_distance``). That code was removed when DSAS
transect/intersection analysis was delegated to the OpenDSAS CLI; this module
brings the shoreline-similarity metric back as a standalone helper that works
directly on shoreline geometries.

Nothing here touches OpenDSAS -- feed it whatever polylines you already have
(``scet.image.Shoreline`` objects, shapely ``LineString``\\s, ``(N, 2)`` arrays,
or lists of ``(x, y)`` / point-like objects).
"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Iterable, List, Sequence

import numpy as np

__all__ = [
    "resample_polyline",
    "discrete_frechet",
    "modified_frechet",
    "orientation_free_frechet",
    "FrechetRate",
    "frechet_change_rates",
    "flag_outliers",
]


def _as_xy(line: Any) -> np.ndarray:
    """Coerce a polyline into an ``(N, 2)`` float array of x/y coordinates.

    Accepts ``scet.image.Shoreline`` (anything with ``.vertices``), shapely
    geometries (anything with ``.coords``), sequences of point-like objects
    exposing ``.x``/``.y``, and plain ``(N, 2)`` array-likes.
    """
    if hasattr(line, "vertices"):
        line = line.vertices
    if hasattr(line, "coords"):
        return np.asarray([(pt[0], pt[1]) for pt in line.coords], dtype=float)

    pts = list(line)  # type: ignore[arg-type]
    if pts and hasattr(pts[0], "x") and hasattr(pts[0], "y"):
        return np.asarray([(p.x, p.y) for p in pts], dtype=float)

    arr = np.asarray(pts, dtype=float)
    if arr.ndim != 2 or arr.shape[1] < 2:
        raise ValueError(
            "expected a polyline as (N, 2) coordinates, got array of shape "
            f"{arr.shape}"
        )
    return arr[:, :2]


def resample_polyline(line: Any, n: int) -> np.ndarray:
    """Resample ``line`` into ``n`` points spaced evenly along its arc length.

    Endpoints are preserved. Port of C++ ``util::equal_divided_polyline``.
    """
    if n < 2:
        raise ValueError("n must be >= 2")
    xy = _as_xy(line)
    if len(xy) < 2:
        raise ValueError("polyline must have at least 2 vertices")

    seg = np.hypot(np.diff(xy[:, 0]), np.diff(xy[:, 1]))
    total = float(seg.sum())
    out = np.empty((n, 2), dtype=float)
    out[0] = xy[0]
    out[-1] = xy[-1]
    if total == 0.0:
        out[1:-1] = xy[0]
        return out

    step = total / (n - 1)
    acc = 0.0
    cur = 0  # index of the segment (xy[cur] -> xy[cur + 1]) we're walking along
    for i in range(1, n - 1):
        target = i * step
        while cur < len(seg) - 1 and acc + seg[cur] < target:
            acc += seg[cur]
            cur += 1
        local = seg[cur]
        frac = 0.0 if local == 0.0 else (target - acc) / local
        out[i] = xy[cur] + (xy[cur + 1] - xy[cur]) * frac
    return out


def discrete_frechet(line_a: Any, line_b: Any) -> float:
    """Discrete Fréchet distance between two polylines (Eiter & Mannila).

    Coupling measure over the given vertices -- no resampling. Port of C++
    ``util::frechet_distance``. O(m * n) time.
    """
    a = _as_xy(line_a)
    b = _as_xy(line_b)
    if len(a) == 0 or len(b) == 0:
        raise ValueError("both polylines must be non-empty")
    n = len(b)

    # Row of the coupling matrix for the previous i.
    prev = np.maximum.accumulate(np.hypot(b[:, 0] - a[0, 0], b[:, 1] - a[0, 1]))
    for i in range(1, len(a)):
        di = np.hypot(b[:, 0] - a[i, 0], b[:, 1] - a[i, 1])
        cur = np.empty(n, dtype=float)
        cur[0] = max(prev[0], di[0])
        for j in range(1, n):
            cur[j] = max(min(prev[j], prev[j - 1], cur[j - 1]), di[j])
        prev = cur
    return float(prev[-1])


def modified_frechet(line_a: Any, line_b: Any) -> float:
    """Spread of the point-to-point gap after equal-arc-length resampling.

    Both lines are resampled to ``max(len(a), len(b))`` equally spaced points;
    the result is ``max(gap) - min(gap)`` over matched point pairs, i.e. how
    much the separation between the two shorelines varies along their length.
    Port of C++ ``util::modified_frechet_distance``.
    """
    a = _as_xy(line_a)
    b = _as_xy(line_b)
    n = max(len(a), len(b))
    if n < 2:
        raise ValueError("polylines must have at least 2 vertices")
    ra = resample_polyline(a, n)
    rb = resample_polyline(b, n)
    gap = np.hypot(ra[:, 0] - rb[:, 0], ra[:, 1] - rb[:, 1])
    return float(gap.max() - gap.min())


def orientation_free_frechet(line_a: Any, line_b: Any) -> float:
    """``modified_frechet`` made independent of vertex ordering.

    Digitised shorelines are not guaranteed to run the same direction, so this
    takes the smaller of the metric computed with ``line_a`` as given and with
    its vertices reversed -- matching the ``std::min(dist1, dist2)`` step in
    C++ ``dsas::frechet_distance``.
    """
    a = _as_xy(line_a)
    b = _as_xy(line_b)
    forward = modified_frechet(a, b)
    reverse = modified_frechet(a[::-1], b)
    return min(forward, reverse)


@dataclass
class FrechetRate:
    """One consecutive-pair shoreline-change measurement.

    ``rate`` is ``orientation_free_frechet`` between the two shorelines divided
    by the time gap (``year_end - year_start``), so it carries the units of the
    coordinates per year. Mirrors the C++ ``set_frechet_info`` record.
    """

    year_start: float
    year_end: float
    distance: float
    rate: float


def _year_value(obj: Any) -> float:
    if hasattr(obj, "year"):
        return float(obj.year)
    if hasattr(obj, "toordinal"):  # datetime.date / datetime.datetime
        return obj.toordinal() / 365.25
    return float(obj)  # already a number


def frechet_change_rates(
    shorelines: Sequence[Any],
    dates: Sequence[Any] | None = None,
) -> List[FrechetRate]:
    """Fréchet-based change rate for each consecutive pair of dated shorelines.

    ``shorelines`` should already be clipped to the stretch of coast you care
    about (the C++ code did this by truncating each shoreline between the first
    and last transect of a group). Pairs are formed after sorting by date.

    ``dates`` overrides the date carried by each shoreline; entries may be
    ``datetime.date``, ``datetime.datetime``, or numbers (treated as years).
    Shoreline objects are read via a ``.year`` attribute otherwise.
    """
    lines = list(shorelines)
    if len(lines) < 2:
        return []
    if dates is not None:
        if len(dates) != len(lines):
            raise ValueError("dates and shorelines must be the same length")
        years = [_year_value(d) for d in dates]
    else:
        years = [_year_value(s) for s in lines]

    order = sorted(range(len(lines)), key=lambda i: years[i])
    lines = [lines[i] for i in order]
    years = [years[i] for i in order]

    out: List[FrechetRate] = []
    for i in range(len(lines) - 1):
        gap = years[i + 1] - years[i]
        if gap == 0:
            continue
        dist = orientation_free_frechet(lines[i], lines[i + 1])
        out.append(FrechetRate(years[i], years[i + 1], dist, dist / gap))
    return out


def flag_outliers(
    values: Iterable[float],
    outlier_rate: float = 3.0,
    two_sided: bool = False,
) -> List[bool]:
    """Flag values that sit more than ``outlier_rate`` sample stdevs from the mean.

    Default (``two_sided=False``) only flags values *above* the mean, matching
    the ``is_fre_outlier`` test ``(x - mean) > outlier_rate * std`` in C++
    ``dsas::frechet_distance``. ``two_sided=True`` flags either tail, matching
    ``util::remove_outliers``. Uses the sample standard deviation (``ddof=1``);
    fewer than two values, or zero spread, flags nothing.
    """
    arr = np.asarray(list(values), dtype=float)
    if len(arr) < 2:
        return [False] * len(arr)
    mean = arr.mean()
    std = arr.std(ddof=1)
    if std == 0.0:
        return [False] * len(arr)
    delta = np.abs(arr - mean) if two_sided else (arr - mean)
    return (delta > outlier_rate * std).tolist()
