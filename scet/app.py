import os
import shutil
from typing import Optional

import pandas as pd

from .extract_merge_data import extract_NAIP_folder, merge_detect_folder
from .options import Options
from .pipeline import generate_result_from_folder, generate_result_from_image
from .water_level_calibration import (
    calculate_rate,
    extract_transect_line,
    load_shapefile,
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

    def process_folder(self, input_naip_zipfiles_folder,
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

    def process_image(self, input_img_path, output_folder):
        from .simple_mmseg.DL_running import process_single_img

        checkpoint_file = self.config.checkpoint_file or download_weights()

        output_img_path = os.path.join(output_folder, "output.png")
        process_single_img(input_img_path,
                           output_img_path,
                           checkpoint_file)
        generate_result_from_image(input_img_path,
                                   output_folder,
                                   self.config.options)

    def calibrate_water_level(self, transect_path, intersect_path, water_levels,
                               slope_field="slope", reference_level=None):
        """Calibrate DSAS ChangeRates for water-level fluctuations using a
        linear beach-slope model.

        A change in water level shifts the observed waterline by
        -(water_level - reference_level) / slope along the transect
        (shallower slope -> bigger shift for the same water-level change).
        Fitting a rate to that predicted shift across each transect's
        shoreline dates gives the portion of the raw ChangeRate caused by
        water-level trends alone; subtracting it from ChangeRate leaves the
        calibrated (water-level-independent) rate.

        transect_path / intersect_path are the transect.shp / intersection.shp
        files dsas_cli.cal writes (see pipeline.py): transect_path has one row
        per transect with ChangeRate and slope_field fields, intersect_path
        has one row per (transect, shoreline date).

        water_levels maps shoreline year -> water level, one entry per image
        year used in the DSAS run. reference_level defaults to the mean of
        water_levels.
        """
        transects, t_crs = load_shapefile(os.path.dirname(transect_path),
                                          os.path.basename(transect_path))
        intersection, i_crs = load_shapefile(os.path.dirname(intersect_path),
                                             os.path.basename(intersect_path))

        if t_crs != i_crs:
            print(f"Warning: transect and intersection are not the same prj\n"
                 f"    transect: {t_crs}\n    intersect: {i_crs}")

        if reference_level is None:
            reference_level = sum(water_levels.values()) / len(water_levels)

        result_df = transects[["BaselineId", "TransectId", "ChangeRate", slope_field]].copy()
        result_df["calibrated_rate"] = 0.0

        for i in range(len(result_df)):
            bid = result_df["BaselineId"].iloc[i]
            tid = result_df["TransectId"].iloc[i]
            slope = result_df[slope_field].iloc[i]

            intersect_sel = extract_transect_line(
                intersection, baseline_id=bid, transect_id=tid)

            years = [int(pd.to_datetime(d).year) for d in intersect_sel["Date"]]
            time_intervals = [y - min(years) for y in years]
            predicted_dists = [
                -(water_levels[y] - reference_level) / slope for y in years
            ]

            try:
                water_level_rate = calculate_rate(
                    time_intervals, predicted_dists)
            except ValueError as e:
                print(f"Warning: {bid}, {tid} with {e}")
                water_level_rate = 0

            orig_rate = result_df["ChangeRate"].iloc[i]
            calibrated_rate = orig_rate - water_level_rate
            result_df.iloc[i, -1] = calibrated_rate

        return result_df
