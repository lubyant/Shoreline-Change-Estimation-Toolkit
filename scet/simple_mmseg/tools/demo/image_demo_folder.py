# Copyright (c) OpenMMLab. All rights reserved.
from argparse import ArgumentParser

from mmengine.model import revert_sync_batchnorm

from mmseg.apis import inference_model, init_model, show_result_pyplot

import os

def get_all_subfolders(folder_path):
    subfolders = []
    for root, dirs, files in os.walk(folder_path):
        for dir_name in dirs:
            subfolders.append(os.path.join(root, dir_name))
    return subfolders

def get_all_files_with_suffix(directory, suffix):
    # List all files in the given directory
    files = os.listdir(directory)
    # Filter files that end with the given suffix
    matched_files = [file for file in files if file.endswith(suffix)]
    return matched_files

def load_model(args):
    model = init_model(args.config, args.checkpoint, device=args.device)
    return model

def main_args(args, model = None):
    # build the model from a config file and a checkpoint file
    if model == None:
        model = init_model(args.config, args.checkpoint, device=args.device)
    if args.device == 'cpu':
        model = revert_sync_batchnorm(model)
    all_imgs = get_all_files_with_suffix(args.img_folder, args.img_suffix)
    for img in all_imgs:
    	# test a single image
    	img_path = args.img_folder + img
    	output_path = args.out_folder + img
    	result = inference_model(model, img_path)
    	# show the results
    	show_result_pyplot(
        	model,
        	img_path,
        	result,
        	title=args.title,
        	opacity=args.opacity,
        	with_labels=args.with_labels,
        	draw_gt=True,
        	show=False if args.out_folder is not None else True,
        	out_file=output_path)

def main():
    parser = ArgumentParser()
    parser.add_argument('--img_folder', help='Image folder', default = '/home/weiwang/ResearchProjects/mmsegmentation/demo/testing/')
    parser.add_argument('--config', help='Config file', default = '/home/weiwang/ResearchProjects/mmsegmentation/my_model/deeplabv3plus/deeplabv3plus_r50-d8_4xb4-20k_voc12aug-512x512.py')
    parser.add_argument('--img_suffix', help='Image suffix', default = 'png')
    parser.add_argument('--checkpoint', help='Checkpoint file', default = '/home/weiwang/ResearchProjects/mmsegmentation/my_model_res/deeplabv3plus/iter_100000.pth')
    parser.add_argument('--out_folder', help='Path to output file', default = '/home/weiwang/ResearchProjects/mmsegmentation/my_model_res/deeplabv3plus/demo_res/testing_v1/')
    parser.add_argument(
        '--device', default='cuda:0', help='Device used for inference')
    parser.add_argument(
        '--opacity',
        type=float,
        default=0.5,
        help='Opacity of painted segmentation map. In (0, 1] range.')
    parser.add_argument(
        '--with-labels',
        action='store_true',
        default=False,
        help='Whether to display the class labels.')
    parser.add_argument(
        '--title', default='result', help='The image identifier.')
    args = parser.parse_args()
    
    # build the model from a config file and a checkpoint file
    model = init_model(args.config, args.checkpoint, device=args.device)
    if args.device == 'cpu':
        model = revert_sync_batchnorm(model)
    all_imgs = get_all_files_with_suffix(args.img_folder, args.img_suffix)
    for img in all_imgs:
    	# test a single image
    	img_path = args.img_folder + img
    	output_path = args.out_folder + img
    	result = inference_model(model, img_path)
    	# show the results
    	show_result_pyplot(
        	model,
        	img_path,
        	result,
        	title=args.title,
        	opacity=args.opacity,
        	with_labels=args.with_labels,
        	draw_gt=True,
        	show=False if args.out_folder is not None else True,
        	out_file=output_path)


if __name__ == '__main__':
    main()
