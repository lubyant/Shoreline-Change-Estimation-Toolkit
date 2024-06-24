from DL_running import process_img_folder
from extract_merge_data import (
    extract_zip, check_folder_exist, get_merged_data_by_point)
import os
from cppext import cppext, Options


class Config:
    def __init__(self, config_path, checkpoint_file) -> None:
        pass


class SCET:
    def __init__(self, config) -> None:
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
        self._check_folders_exist(output_raster_folder,
                                  output_divided_img_folder,
                                  output_infrared_folder,
                                  output_pkl_folder,
                                  output_txt_folder,
                                  output_binary_map_folder,
                                  final_save_folder,
                                  output_result_folder)

        extract_zip(input_naip_zipfiles_folder,
                    output_raster_folder,
                    output_divided_img_folder,
                    output_pkl_folder,
                    output_infrared_folder,
                    output_txt_folder)

        process_img_folder(output_divided_img_folder,
                           output_binary_map_folder,
                           self.config.config_file,
                           self.config.checkpoint_file)

        self._merge_raster(output_binary_map_folder,
                           output_pkl_folder,
                           final_save_folder)

        self._shoreline_analysis(final_save_folder,
                                 output_result_folder)

    def method2(self):
        pass

    def _check_folders_exist(self, *args):
        for folder in args:
            check_folder_exist(folder)

    def _merge_raster(self, binary_folder,
                      pkl_folder, final_output_folder):
        years = os.listdir(binary_folder)
        for year in years:
            image_ids = os.listdir(
                os.path.join(binary_folder, year)
            )
            for image_id in image_ids:
                img_folder = os.path.join(binary_folder, year, image_id)
                pkl_folder = os.path.join(pkl_folder, year, image_id)
                get_merged_data_by_point(img_folder, pkl_folder,
                                         final_output_folder,
                                         image_id, year)

    def _shoreline_analysis(self, final_output_folder,
                            output_result_folder):
        options = Options()
        cppext(final_output_folder, output_result_folder, options)
