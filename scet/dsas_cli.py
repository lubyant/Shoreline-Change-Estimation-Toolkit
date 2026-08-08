"""Thin subprocess wrapper around the OpenDSAS `dsas` CLI.

Replaces the transect-casting, shoreline/transect intersection, and
linear-regression change-rate code that used to live in geometry.cpp and
shorelinecalculator.cpp -- that logic is a duplicate of what OpenDSAS
(https://github.com/lubyant/OpenDSAS, `pip install opendsas`) already does.
"""
from __future__ import annotations

import shutil
import subprocess
from pathlib import Path
from typing import List

from .options import Options

DSAS_EXECUTABLE = "dsas"


def find_executable() -> str:
    exe = shutil.which(DSAS_EXECUTABLE)
    if exe is None:
        raise RuntimeError(
            "The 'dsas' executable was not found on PATH. Install it with "
            "`pip install opendsas` (https://pypi.org/project/opendsas/)."
        )
    return exe


def cast(
    baseline_path: Path | str,
    output_transect_path: Path | str,
    options: Options,
    bid_field: str = "Id",
) -> None:
    """Generate transects from a baseline via `dsas cast`."""
    cmd: List[str] = [
        find_executable(),
        "cast",
        "--baseline",
        str(baseline_path),
        "--bid-field",
        bid_field,
        "--output-transect",
        str(output_transect_path),
        "--transect-length",
        str(options.transect_length),
        "--transect-spacing",
        str(options.transect_spacing),
        "--smooth-factor",
        str(options.smooth_factor),
        "--intersection-mode",
        options.intersection_mode.value,
        "--transect-orientation",
        options.transect_orient.value,
    ]
    subprocess.run(cmd, check=True)


def cal(
    transect_path: Path | str,
    shoreline_path: Path | str,
    output_intersect_path: Path | str,
    options: Options,
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> None:
    """Compute shoreline/transect intersections and change rates via `dsas cal`."""
    cmd: List[str] = [
        find_executable(),
        "cal",
        "--transect",
        str(transect_path),
        "--shoreline",
        str(shoreline_path),
        "--date-field",
        date_field,
        "--date-format",
        date_format,
        "--intersection-mode",
        options.intersection_mode.value,
        "--transect-orientation",
        options.transect_orient.value,
        "--output-intersect",
        str(output_intersect_path),
    ]
    subprocess.run(cmd, check=True)


def run(
    baseline_path: Path | str,
    shoreline_path: Path | str,
    output_transect_path: Path | str,
    output_intersect_path: Path | str,
    options: Options,
    bid_field: str = "Id",
    date_field: str = "Date",
    date_format: str = "%Y/%m/%d",
) -> None:
    """Run the full cast+cal pipeline via the root `dsas` command in one call."""
    cmd: List[str] = [
        find_executable(),
        "--baseline",
        str(baseline_path),
        "--bid-field",
        bid_field,
        "--shoreline",
        str(shoreline_path),
        "--date-field",
        date_field,
        "--date-format",
        date_format,
        "--output-transect",
        str(output_transect_path),
        "--output-intersect",
        str(output_intersect_path),
        "--smooth-factor",
        str(options.smooth_factor),
        "--transect-length",
        str(options.transect_length),
        "--transect-spacing",
        str(options.transect_spacing),
        "--intersection-mode",
        options.intersection_mode.value,
        "--transect-orientation",
        options.transect_orient.value,
    ]
    subprocess.run(cmd, check=True)
