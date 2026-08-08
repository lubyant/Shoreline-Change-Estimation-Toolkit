"""Configuration shared by the image-extraction pipeline and OpenDSAS CLI calls."""
from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Any


class IntersectionMode(str, Enum):
    CLOSEST = "closest"
    FARTHEST = "farthest"


class TransectOrientation(str, Enum):
    LEFT = "left"
    RIGHT = "right"
    MIX = "mix"


@dataclass
class Options:
    # image -> shoreline extraction
    edge_distance: int = 100
    shoreline_least_factor: float = 0.5

    # forwarded to `dsas cast` / `dsas cal`
    smooth_factor: int = 1
    transect_length: float = 500
    transect_spacing: float = 30
    intersection_mode: IntersectionMode = IntersectionMode.CLOSEST
    transect_orient: TransectOrientation = TransectOrientation.MIX

    @classmethod
    def from_json(cls, json_value: dict[str, Any]) -> "Options":
        data = json_value.get("options", json_value)
        kwargs: dict[str, Any] = {}
        if "smooth_factor" in data:
            kwargs["smooth_factor"] = int(data["smooth_factor"])
        if "edge_distance" in data:
            kwargs["edge_distance"] = int(data["edge_distance"])
        if "shoreline_least_factor" in data:
            kwargs["shoreline_least_factor"] = float(data["shoreline_least_factor"])
        if "transect_length" in data:
            kwargs["transect_length"] = float(data["transect_length"])
        if "transect_spacing" in data:
            kwargs["transect_spacing"] = float(data["transect_spacing"])
        if "intersection_mode" in data:
            kwargs["intersection_mode"] = IntersectionMode(data["intersection_mode"])
        if "transect_orientation" in data:
            kwargs["transect_orient"] = TransectOrientation(data["transect_orientation"])
        return cls(**kwargs)
