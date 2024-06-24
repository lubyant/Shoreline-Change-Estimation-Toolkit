from tools.demo.image_demo_folder import main_args, get_all_files_with_suffix,  \
                                                      get_all_subfolders, load_model
from tools.demo.image_demo import single_img_main_args
from tools.customize_tools.image_demo_config import image_folder_demo_config, image_demo_config
import os
import argparse

def process_img_folder(img_folder, output_folder, config_file, checkpoint_file, img_suffix = 'png'):
    """_summary_
    This is a function to process img_folder, use DL model to extract water body, and output a binary image of segmentation results.
    img_folder can be a folder with images, it can also be a folder with many subfolders, and the images are saved within each subfolder.
    However, to avoid errors, please do not mix image subfolders with image directly saved in img_folder.
    
    Args:
        img_folder (str): input image folder
        output_folder (str): output image folder, if input image folder is a folder of subfolders, this is the root folder for output images
        config_file (str): path to DL model config file
        checkpoint_file (str): path to trained DL checkpoint file
        img_suffix (str, optional): suffix of image file. Defaults to 'png'.
    """
    os.makedirs(output_folder, exist_ok=True)
    if get_all_files_with_suffix(img_folder, img_suffix):
        args = image_folder_demo_config(img_folder, config_file, checkpoint_file, output_folder, img_suffix = img_suffix)
        model = load_model(args)
        main_args(args, model = model)
    else:
        sub_folders = get_all_subfolders(img_folder)
        count = 0
        for s in sub_folders:
            if get_all_files_with_suffix(s, img_suffix):
                
                input_folder = s + '/'
                save_folder = output_folder + '/' + s.split('/')[-2] + '/' + s.split('/')[-1] + '/'
                args = image_folder_demo_config(input_folder, config_file, checkpoint_file, save_folder, img_suffix = img_suffix)
                if count == 0:
                    
                    model = load_model(args)
                main_args(args, model = model)
                count += 1


def process_single_img(img_file, out_file, config_file, checkpoint_file, img_suffix = 'png'):
    """
    This is a function for demoing an image using trained DL model
    Args:
        img_file (str): path to input image file
        out_file (str): output image path
        config_file (str): path to DL model config file
        checkpoint_file (str): path to trained DL checkpoint file
        img_suffix (str, optional): suffix of image file. Defaults to 'png'.
    """
    # print(out_file)
    output_folder = '/'.join(out_file.split('/')[:-1])
    os.makedirs(output_folder, exist_ok=True)
    args = image_demo_config(img_file, config_file, checkpoint_file, out_file, img_suffix = img_suffix)
    print(args.config)
    model = load_model(args)
    single_img_main_args(args, model = model)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'Environment Settings', formatter_class = argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-imp', '--input_img_path', dest = 'input_img_path', type = str, \
                        default = '/media/weiwang/easystore/NAIP/Divided_Img/LakeZero/', help = 'Path to divided img(s)')
    parser.add_argument('-omp', '--out_img_path', dest = 'out_img_path', type = str, \
                        default = '/media/weiwang/easystore/NAIP/Marked_Img/LakeZero/', help = 'path to save detected img(s)')
    parser.add_argument('-config', '--config_file', dest = 'config_file', type = str, \
                        default = './my_model/deeplabv3plus/deeplabv3plus_r50-d8_4xb4-20k_voc12aug-512x512.py',
                        help = 'path to save the config file')
    parser.add_argument('-suffix', '--suffix', dest = 'suffix', default = 'png', help = 'the suffix of img files')
    parser.add_argument('-checkpoint', '--checkpoint_file', dest = 'checkpoint_file', type = str, \
                        default = '/mnt/ssd1/mmseg_backup/model_res_wo_05-07/iter_100000.pth', help = 'path to save model checkpoint')
    args = parser.parse_args()
    if os.path.isdir(args.input_img_path):
        process_img_folder(args.input_img_path, args.out_img_path, args.config_file, args.checkpoint_file, img_suffix = args.suffix)
    elif os.path.isfile(args.input_img_path):
        process_single_img(args.input_img_path, args.out_img_path, args.config_file, args.checkpoint_file, img_suffix = args.suffix)
