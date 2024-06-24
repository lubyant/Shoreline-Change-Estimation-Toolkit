from DL_running import process_img_folder
from extract_merge_data import extract_zip, check_folder_exist
import os


class SCET:
    def __init__(self, config) -> None:
        self.config = config

    def method1(self, input_naip_zipfiles_folder,
                output_folder):
        img_folder = self._extract_zipfiles(input_naip_zipfiles_folder,
                                            output_folder)
        self._image_segmentation(img_folder)

    def method2(self):
        pass

    def _check_folders_exist(self, *args):
        for folder in args:
            check_folder_exist(folder)

    def _extract_zipfiles(self, input_naip_zipfiles_folder,
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
        self._check_folders_exist(output_raster_folder,
                                  output_divided_img_folder,
                                  output_infrared_folder,
                                  output_pkl_folder,
                                  output_txt_folder)
        extract_zip(input_naip_zipfiles_folder,
                    output_raster_folder,
                    output_divided_img_folder,
                    output_pkl_folder,
                    output_infrared_folder,
                    output_txt_folder)
        return output_divided_img_folder

    def _image_segmentation(self, img_folder, output_folder):
        process_img_folder(img_folder, output_folder,
                           self.config.config_file,
                           self.config.checkpoint_file)

    def _merge_raster():
        pass
