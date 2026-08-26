import os
import shutil
from typing import Optional

import pandas as pd

from .extract_merge_data import extract_NAIP_folder, merge_detect_folder
from .options import Options, TransectOrientation
from .pipeline import generate_result_from_folder, generate_result_from_image
from .water_level_calibration import (
    calculate_rate,
    extract_raster_profile_from_metadata,
    extract_transect_line,
    find_elevation_intersections,
    load_raster,
    load_shapefile,
    read_excel,
    refine_intersection_points,
)
from .weights import download_weights

# simple_mmseg pulls in torch/mmengine/mmcv, which are optional (extras_require
# "dl"). Import lazily so `import scet` works without them installed.


class Config:
    def __init__(self,
                 checkpoint_file: Optional[str] = None,
                 transect_spacing: float = 30.0,
                 transect_length: float = 500.0) -> None:

        # None defers to the pretrained checkpoint, downloaded on first use.
        self.checkpoint_file = checkpoint_file
        self.keep_raster = False
        self.options = Options()
        self.options.transect_length = transect_length
        self.options.transect_spacing = transect_spacing


class SCET:
    def __init__(self, config: Config) -> None:
        self.config = config

    def method1(self, input_naip_zipfiles_folder,
                output_folder):
        from .simple_mmseg.DL_running import process_img_folder

        checkpoint_file = self.config.checkpoint_file or download_weights()

        output_raster_folder = os.path.join(output_folder,
                                            "Raster")
        output_divided_img_folder = os.path.join(output_folder,
                                                 "Divided_Img")
        output_infrared_folder = os.path.join(output_folder,
                                              "Infrared")
        output_pkl_folder = os.path.join(output_folder,
                                         "Pckl_Folder")
        output_txt_folder = os.path.join(output_folder,
                                         "TXT_Folder")
        output_binary_map_folder = os.path.join(output_folder,
                                                "binary_map")
        final_save_folder = os.path.join(output_folder,
                                         "Final_Folder")
        output_result_folder = os.path.join(output_folder,
                                            "Result")

        extract_NAIP_folder(input_naip_zipfiles_folder,
                            output_raster_folder,
                            output_divided_img_folder,
                            output_infrared_folder,
                            output_pkl_folder,
                            output_txt_folder)

        process_img_folder(output_divided_img_folder,
                           output_binary_map_folder,
                           checkpoint_file)

        merge_detect_folder(output_binary_map_folder,
                            output_pkl_folder,
                            final_save_folder)

        generate_result_from_folder(final_save_folder,
                                    output_result_folder,
                                    self.config.options)

        shutil.rmtree(output_raster_folder)
        shutil.rmtree(output_divided_img_folder)
        shutil.rmtree(output_infrared_folder)
        shutil.rmtree(output_pkl_folder)
        if not self.config.keep_raster:
            shutil.rmtree(output_binary_map_folder)
        shutil.rmtree(final_save_folder)

    def method2(self, input_img_path, output_folder):
        from .simple_mmseg.DL_running import process_single_img

        checkpoint_file = self.config.checkpoint_file or download_weights()

        output_img_path = os.path.join(output_folder, "output.png")
        process_single_img(input_img_path,
                           output_img_path,
                           checkpoint_file)
        generate_result_from_image(input_img_path,
                                   output_folder,
                                   self.config.options)

    def method3(self, transect_path, intersect_path, bathy_raster_path,
                water_level_path):
        """Calibrate DSAS ChangeRates against a bathymetric raster and a
        water-level time series.

        For each transect, find where the raster's elevation profile crosses
        each year's water level, compare that to the year's shoreline
        intersection distance along the transect, fit a rate from the
        residuals, and subtract it from the transect's raw ChangeRate.

        transect_path / intersect_path are the transect.shp / intersection.shp
        files dsas_cli.cal writes (see pipeline.py): transect_path has one row
        per transect with a ChangeRate field, intersect_path has one row per
        (transect, shoreline date) with a ref_dist field.
        """
        transects, t_crs = load_shapefile(os.path.dirname(transect_path),
                                          os.path.basename(transect_path))
        intersection, i_crs = load_shapefile(os.path.dirname(intersect_path),
                                             os.path.basename(intersect_path))
        raster_data, r_crs, meta = load_raster(os.path.dirname(bathy_raster_path),
                                               os.path.basename(bathy_raster_path))

        if t_crs != i_crs:
            print(f"Warning: transect and intersection are not the same prj\n"
                 f"    transect: {t_crs}\n    intersect: {i_crs}")

        if r_crs != i_crs:
            print(f"Warning: raster and intersection are not the same prj\n"
                 f"    raster: {r_crs}\n    intersect: {i_crs}")

        water_level_data = read_excel(water_level_path)

        # extract_raster_profile_from_metadata() returns distances measured
        # from the transect line's start point, but ref_dist (in
        # intersect_path) is measured from the baseline. Where the baseline
        # sits along the transect depends on transect_orient: cast() puts it
        # at the far end for LEFT, at the start for RIGHT, and at the
        # midpoint for MIX.
        orient = self.config.options.transect_orient
        transect_length = self.config.options.transect_length
        if orient == TransectOrientation.LEFT:
            def to_baseline_dist(d):
                return transect_length - d
        elif orient == TransectOrientation.RIGHT:
            def to_baseline_dist(d):
                return d
        else:
            def to_baseline_dist(d):
                return transect_length / 2 - d

        result_df = transects[["BaselineId", "TransectId", "ChangeRate"]].copy()
        result_df["calibrated_rate"] = 0.0

        for i in range(len(result_df)):
            print(f"{i+1}/{len(result_df)}")
            bid = result_df["BaselineId"].iloc[i]
            tid = result_df["TransectId"].iloc[i]

            transect_line = extract_transect_line(
                transects, baseline_id=bid, transect_id=tid).geometry.iloc[0]
            elev_vals, dists = extract_raster_profile_from_metadata(
                raster_data, meta, transect_line, num_points=100)

            intersect_sel = extract_transect_line(
                intersection, baseline_id=bid, transect_id=tid)

            refined_dists, refined_time_intervals = [], []
            for j in range(len(intersect_sel)):
                temp_intersect = intersect_sel.iloc[j]
                temp_year = int(pd.to_datetime(temp_intersect["Date"]).year)
                temp_dist = float(temp_intersect["ref_dist"])

                site_val = float(water_level_data.loc[
                    water_level_data["Year"] == temp_year, " Water Level"].iloc[0])
                target_dists = find_elevation_intersections(
                    elev_vals, dists, site_val)
                target_dists = [to_baseline_dist(x) for x in target_dists]
                refined_dist = refine_intersection_points(
                    target_dists, temp_dist)
                refined_dists.append(refined_dist)
                refined_time_intervals.append(temp_year - 2000)

            try:
                water_level_rate = calculate_rate(
                    refined_time_intervals, refined_dists)
            except ValueError as e:
                print(f"Warning: {bid}, {tid} with {e}")
                water_level_rate = 0

            orig_rate = result_df["ChangeRate"].iloc[i]
            calibrated_rate = orig_rate - water_level_rate
            result_df.iloc[i, -1] = calibrated_rate

        return result_df
