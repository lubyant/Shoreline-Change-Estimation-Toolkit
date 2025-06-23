import os
import shutil

from cppext import Options, generate_result_from_folder, generate_result_from_image

from .extract_merge_data import extract_NAIP_folder, merge_detect_folder
from .simple_mmseg.DL_running import process_img_folder, process_single_img
from .water_level_calibration import (load_shapefile, load_raster, read_excel, extract_transect_line, extract_raster_profile_from_metadata, find_elevation_intersections, refine_intersection_points, days_difference, get_verified_value)


class Config:
    def __init__(self,
                 checkpoint_file,
                 transect_spacing: float = 30.0,
                 transect_length: float = 500.0) -> None:

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
                           self.config.checkpoint_file)

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
        output_img_path = os.path.join(output_folder, "output.png")
        process_single_img(input_img_path,
                           output_img_path,
                           self.config.checkpoint_file)
        generate_result_from_image(input_img_path,
                                   output_folder,
                                   self.config.options)

    def method3(self, transect_path, intersect_path, bathy_raster_path,
                     water_level_path):
        transects, t_crs = load_shapefile(os.path.dirname(transect_path), 
                                   os.path.basename(transect_path))
        intersection, i_crs = load_shapefile(os.path.dirname(intersect_path),
                                      os.path.basename(intersect_path))
        raster_data, r_crs, meta = load_raster(os.path.dirname(bathy_raster_path), 
                                            os.path.basename(bathy_raster_path))
        
        if t_crs != i_crs:
            raise ValueError(f"transect and intersection are not the same prj\
                transect: {t_crs}, intersect: {i_crs}")

        if r_crs != i_crs:
            raise ValueError(f"raster and intersection are not the same prj\
                raster: {r_crs}, intersect: {i_crs}")

        water_level_data = read_excel(water_level_path)
        
        line_to_analysis = extract_transect_line(transects)

        elev_vals, dists = extract_raster_profile_from_metadata(
            raster_data, meta, line_to_analysis, num_points=100
        )
        print(elev_vals)
        print(dists)

        # refined_dists, refined_time_intervals = [], []
        # for i in range(len(line_to_analysis)):
        #     temp_intersect = line_to_analysis.iloc[i]
        #     temp_year = temp_intersect["Year"]
        #     temp_dist = temp_intersect["Dist"]
        #     site_date = site_date_info[str(site_num)].get(str(temp_year))
        #     site_val = float(get_verified_value(water_level_data, site_date))
        #     target_dists = find_elevation_intersections(elev_vals, dists, site_val)
        #     target_dists = [300 - x for x in target_dists]
        #     refined_dist = refine_intersection_points(target_dists, temp_dist)
        #     refined_dists.append(refined_dist)
        #     refined_time_intervals.append(days_difference("20000101", site_date))
