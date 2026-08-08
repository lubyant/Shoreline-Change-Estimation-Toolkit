"""Shoreline extraction from segmented raster imagery.

Pure-Python replacement for image.cpp/hpp and the parts of geometry.hpp
(Point, GeoInfo, Shoreline) that only exist to support it. Baseline/transect/
intersection/rate computation used to live alongside this in C++ but is a
duplicate of what the OpenDSAS CLI (`dsas`) already does -- see dsas_cli.py
and pipeline.py.
"""
from __future__ import annotations

import datetime as dt
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, List, Optional

import cv2
import numpy as np
import rasterio
from pyproj import Transformer

from .options import Options


@dataclass
class Point:
    x: float
    y: float


@dataclass
class GeoInfo:
    """Georeferencing + pixel-extent info for one raster."""

    up_left: Point = field(default_factory=lambda: Point(-1.0, -1.0))
    up_right: Point = field(default_factory=lambda: Point(-1.0, -1.0))
    bottom_left: Point = field(default_factory=lambda: Point(-1.0, -1.0))
    bottom_right: Point = field(default_factory=lambda: Point(-1.0, -1.0))
    rows: int = 0
    cols: int = 0
    pixel_size_x: float = 0.0
    pixel_size_y: float = 0.0
    rotation_x: float = 0.0
    rotation_y: float = 0.0

    def is_overlaid(self, other: "GeoInfo") -> bool:
        x_overlap = (self.bottom_right.x >= other.bottom_left.x) and (
            other.bottom_right.x >= self.bottom_left.x
        )
        y_overlap = (self.up_left.y >= other.bottom_left.y) and (
            other.up_left.y >= self.bottom_left.y
        )
        return x_overlap and y_overlap

    def contains_point(self, point: Point) -> bool:
        x_ok = self.bottom_left.x <= point.x <= self.bottom_right.x
        y_ok = self.bottom_left.y <= point.y <= self.up_right.y
        return x_ok and y_ok


@dataclass
class Shoreline:
    shoreline_id: int
    year: int
    image_id: int
    geo_info: GeoInfo
    date: dt.date
    vertices: List[Point] = field(default_factory=list)

    def __len__(self) -> int:
        return len(self.vertices)


def _get_raster_proj_wkt(path: Path) -> str:
    with rasterio.open(path) as ds:
        if ds.crs is None:
            raise RuntimeError(f"No CRS found in raster: {path}")
        return ds.crs.to_wkt()


def _make_transform(source_wkt: str, target_wkt: str) -> Transformer:
    # always_xy=True keeps (x, y) = (easting/lon, northing/lat) point order
    # regardless of the authority-defined axis order, matching the GDAL
    # 2-style behavior the original C++ relied on.
    return Transformer.from_crs(source_wkt, target_wkt, always_xy=True)


class Image:
    """Extracts shoreline polylines from a single segmented raster image.

    The input raster is expected to be a binary land/water mask (as produced
    upstream by the segmentation step): non-zero pixels are "land", and the
    boundary between zero and non-zero pixels is the shoreline.
    """

    def __init__(
        self,
        image_path: Path | str,
        options: Options,
        image_id: Optional[str] = None,
        date: Optional[dt.date] = None,
    ) -> None:
        self.image_path = Path(image_path)
        self.options = options
        self.edge_distance = options.edge_distance
        self.least_factor = options.shoreline_least_factor

        stem = self.image_path.stem
        if image_id is not None and date is not None:
            self.file_name = image_id
            self.image_id = int(image_id)
            self.date = date
            self.year = date.year
        else:
            try:
                self.year = int(stem[-4:])
            except ValueError as exc:
                raise ValueError(
                    f"{stem}: cannot parse a 4-digit year from filename"
                ) from exc
            self.date = dt.date(self.year, 1, 1)
            self.file_name = stem[:-4]
            # Mirrors C++'s `istringstream >> int`: parse a leading integer
            # and ignore trailing garbage; default to 0 if there is none.
            match = re.match(r"\s*[-+]?\d+", self.file_name)
            self.image_id = int(match.group()) if match else 0

        self.proj_wkt = _get_raster_proj_wkt(self.image_path)
        self.geo_info = GeoInfo()
        self.shorelines: List[Shoreline] = []

        self._extract_geoinfo()
        contours = self._extract_contours()
        self._extract_shorelines(contours)
        self._process_shorelines()
        self._transform_pixel_to_geo()

    # -- extraction steps -------------------------------------------------

    def _extract_geoinfo(self) -> None:
        with rasterio.open(self.image_path) as ds:
            if ds.transform.is_identity:
                raise RuntimeError(f"No geo-transform found: {self.image_path}")
            gt = ds.transform.to_gdal()

            gi = self.geo_info
            gi.up_left = Point(gt[0], gt[3])
            gi.pixel_size_x, gi.rotation_x = gt[1], gt[2]
            gi.rotation_y, gi.pixel_size_y = gt[4], gt[5]
            gi.cols = ds.width
            gi.rows = ds.height

        gi.bottom_right = Point(
            gi.up_left.x + gi.pixel_size_x * gi.cols,
            gi.up_left.y + gi.pixel_size_y * gi.rows,
        )
        gi.up_right = Point(gi.bottom_right.x, gi.up_left.y)
        gi.bottom_left = Point(gi.up_left.x, gi.bottom_right.y)

    def _extract_contours(self) -> List[np.ndarray]:
        img = cv2.imread(str(self.image_path))
        if img is None:
            raise RuntimeError(f"Cannot read image: {self.image_path}")
        self.geo_info.rows, self.geo_info.cols = img.shape[0], img.shape[1]

        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        _, thresh = cv2.threshold(gray, 1, 255, 0)
        contours, _ = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
        if not contours:
            raise RuntimeError(f"No contours in image: {self.image_path}")

        contours = [c for c in contours if not self._is_closure(c)]
        if not contours:
            raise RuntimeError(f"No edge contours in image: {self.image_path}")
        return contours

    def _is_edge(self, x_cor: int, y_cor: int) -> bool:
        d = self.edge_distance
        return (
            x_cor <= d
            or x_cor >= self.geo_info.cols - d
            or y_cor <= d
            or y_cor >= self.geo_info.rows - d
        )

    def _is_closure(self, contour: np.ndarray) -> bool:
        return not any(self._is_edge(int(pt[0][0]), int(pt[0][1])) for pt in contour)

    def _extract_shorelines(self, contours: Iterable[np.ndarray]) -> None:
        shoreline_id = 0
        shorelines: List[Shoreline] = []

        def new_shoreline() -> Shoreline:
            nonlocal shoreline_id
            s = Shoreline(shoreline_id, self.year, self.image_id, self.geo_info, self.date)
            shoreline_id += 1
            return s

        for contour in contours:
            points = new_shoreline()
            for pt in contour:
                x, y = int(pt[0][0]), int(pt[0][1])
                if self._is_edge(x, y):
                    if points.vertices:
                        shorelines.append(points)
                        points = new_shoreline()
                    continue
                points.vertices.append(Point(x, y))
            if points.vertices:
                shorelines.append(points)

        if not shorelines:
            raise RuntimeError(f"No shorelines from contours: {self.image_path}")
        self.shorelines = shorelines

    def _process_shorelines(self) -> None:
        max_len = max(len(s) for s in self.shorelines)
        threshold = int(self.least_factor * max_len)
        self.shorelines = [s for s in self.shorelines if len(s) >= threshold]
        if not self.shorelines:
            raise RuntimeError(f"All shorelines filtered out: {self.image_path}")

    def _transform_pixel_to_geo(self) -> None:
        gi = self.geo_info
        for shoreline in self.shorelines:
            transformed = []
            for p in shoreline.vertices:
                i, j = p.x, p.y
                x_geo = gi.up_left.x + i * gi.pixel_size_x + j * gi.rotation_x
                y_geo = gi.up_left.y + i * gi.rotation_y + j * gi.pixel_size_y
                transformed.append(Point(x_geo, y_geo))
            shoreline.vertices = transformed

    # -- coordinate transforms / merging -----------------------------------

    def reproject(self, target_wkt: str) -> None:
        """Reproject this image's geo_info and shoreline vertices in place."""
        if target_wkt == self.proj_wkt:
            return
        transform = _make_transform(self.proj_wkt, target_wkt)

        def tf(p: Point) -> Point:
            x, y = transform.transform(p.x, p.y)
            return Point(x, y)

        gi = self.geo_info
        gi.up_left = tf(gi.up_left)
        gi.up_right = tf(gi.up_right)
        gi.bottom_left = tf(gi.bottom_left)
        gi.bottom_right = tf(gi.bottom_right)

        for shoreline in self.shorelines:
            shoreline.geo_info = gi
            shoreline.vertices = [tf(p) for p in shoreline.vertices]
        self.proj_wkt = target_wkt

    def is_overlaid(self, other: "Image") -> bool:
        return self.geo_info.is_overlaid(other.geo_info)

    def joint_shorelines(self, other_images: Iterable["Image"]) -> List[Shoreline]:
        """Vertices of this image's shorelines that fall outside every image
        in `other_images` -- used to de-duplicate shoreline segments across
        overlapping image tiles."""
        others = list(other_images)
        result: List[Shoreline] = []
        for shoreline in self.shorelines:
            kept = [
                p
                for p in shoreline.vertices
                if not any(img.geo_info.contains_point(p) for img in others)
            ]
            if kept:
                result.append(
                    Shoreline(
                        shoreline.shoreline_id,
                        shoreline.year,
                        shoreline.image_id,
                        shoreline.geo_info,
                        shoreline.date,
                        kept,
                    )
                )
        return result


def merge_shorelines_from_images(images: List[Image], proj_wkt: Optional[str] = None) -> List[Shoreline]:
    """Concatenate shorelines from every image, reprojecting onto a common CRS."""
    if not images:
        return []
    target_wkt = proj_wkt or images[0].proj_wkt
    for image in images:
        image.reproject(target_wkt)

    shorelines: List[Shoreline] = []
    for image in images:
        shorelines.extend(image.shorelines)
    return shorelines


def merge_baseline_from_images(images: List[Image]) -> List[Shoreline]:
    """Derive baseline geometry from the non-overlapping union of image
    shorelines (each caller is expected to have already reprojected `images`
    onto a common CRS via merge_shorelines_from_images)."""
    baseline_shorelines: List[Shoreline] = []
    visited: List[Image] = []
    baseline_id = 0
    for image in images:
        for shoreline in image.joint_shorelines(visited):
            if not shoreline.vertices:
                continue
            shoreline.shoreline_id = baseline_id
            baseline_id += 1
            baseline_shorelines.append(shoreline)
        visited.append(image)
    return baseline_shorelines
