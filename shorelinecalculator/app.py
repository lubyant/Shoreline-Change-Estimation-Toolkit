import os
import shutil

from shorelinecalculator.cppext import Options, generate_result_from_folder, generate_result_from_image
from .extract_merge_data import extract_NAIP_folder, merge_detect_folder
from .simple_mmseg.DL_running import process_img_folder, process_single_img


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
        output_raster_folder = os.path.join(output_folder, "Raster")
        output_divided_img_folder = os.path.join(output_folder, "Divided_Img")
        output_infrared_folder = os.path.join(output_folder, "Infrared")
        output_pkl_folder = os.path.join(output_folder, "Pckl_Folder")
        output_txt_folder = os.path.join(output_folder, "TXT_Folder")
        output_binary_map_folder = os.path.join(output_folder, "binary_map")
        final_save_folder = os.path.join(output_folder, "Final_Folder")
        output_result_folder = os.path.join(output_folder, "Result")

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
