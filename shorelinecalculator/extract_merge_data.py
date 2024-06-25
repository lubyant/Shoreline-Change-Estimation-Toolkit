import shutil
import re
import rasterio
import os
import numpy as np

from osgeo import gdal
import cv2
import pickle
from rasterio.transform import Affine
from rasterio.merge import merge
from PIL import Image
import argparse
import zipfile
from pyproj import Proj, transform
import time
import uuid
from tqdm import tqdm


def check_folder_exist(filepath):
    if not os.path.exists(filepath):
        os.mkdir(filepath)


def get_subfolders_with_digits(folder):
    subfolders = [f for f in os.listdir(folder)
                  if os.path.isdir(os.path.join(folder, f)) and f.isdigit()]
    return subfolders


def check_id_year_exist(folder, sample_id, year):
    check_folder_exist(folder + str(year) + '/')
    check_folder_exist(folder + str(year) + '/' + str(sample_id) + '/')


def find_by_suffix(check_folder, check_suffix):
    file_names = []
    for _, _, files in os.walk(check_folder):
        for file in files:
            if file.split('.')[-1] == check_suffix:
                file_names.append(file)
    return file_names


def load_pickle(pickle_folder, pickle_name):
    pickle_dir = pickle_folder + pickle_name
    with open(pickle_dir, 'rb') as f:
        data = pickle.load(f)

    return data


def save_pickle(to_save, pickle_folder, pickle_name):
    check_folder_exist(pickle_folder)
    pickle_dir = pickle_folder + pickle_name
    with open(pickle_dir, 'wb') as f:
        pickle.dump(to_save, f)


def extract_raster_meta(raster_folder, raster_file):
    raster_addr = raster_folder + raster_file
    src = rasterio.open(raster_addr)
    out_meta = src.meta
    out_meta.update({
        # "driver": "GTiff",
        "count": 3,
    })
    return out_meta


def update_onechannel_metadata(out_meta):
    out_meta.update({
        # "driver": "GTiff",
        "count": 1,
    })
    return out_meta


def save_new_raster(out_img, out_meta, raster_folder, raster_name):
    check_folder_exist(raster_folder)
    with rasterio.open(raster_folder + raster_name, "w", **out_meta) as dest:
        dest.write(out_img)


def get_img_from_raster(raster_folder, raster_file):
    ds = gdal.Open(raster_folder + raster_file)
    myarray1 = np.array(ds.GetRasterBand(1).ReadAsArray())
    myarray2 = np.array(ds.GetRasterBand(2).ReadAsArray())
    myarray3 = np.array(ds.GetRasterBand(3).ReadAsArray())
    res = np.stack((np.uint8(myarray3), np.uint8(
        myarray2), np.uint8(myarray1)), axis=2)
    return res


def get_img_source_data_from_raster(raster_folder, raster_file):
    ds = gdal.Open(raster_folder + raster_file)
    return ds.GetRasterBand(1).ReadAsArray(), ds.GetRasterBand(2).ReadAsArray(), ds.GetRasterBand(3).ReadAsArray()


def get_infrared_img_from_raster(raster_folder, raster_file):
    ds = gdal.Open(raster_folder + raster_file)
    myarray1 = np.array(ds.GetRasterBand(4).ReadAsArray())
    return myarray1


def divide_img(img, out_folder, out_name, row_interval=1000, col_interval=1000):
    check_folder_exist(out_folder)
    row_num, col_num = img.shape[:2]
    rows, cols = row_num // row_interval, col_num // col_interval

    for i in range(rows):
        for j in range(cols):
            img_name = '%d_%d_%s' % (i, j, out_name)
            r_start, c_start = i * row_interval, j * col_interval
            r_end = (i + 1) * row_interval if i < rows - 1 else row_num
            c_end = (j + 1) * col_interval if j < cols - 1 else col_num
            # print([r_start, r_end, c_start, c_end])
            if len(img.shape) > 2:
                cv2.imwrite(out_folder + img_name,
                            img[r_start: r_end, c_start: c_end, :])
            else:
                cv2.imwrite(out_folder + img_name,
                            img[r_start: r_end, c_start: c_end])


def divide_img_by_interval(img, out_folder, out_name, row_interval=1000, col_interval=1000):
    check_folder_exist(out_folder)
    row_num, col_num = img.shape[:2]
    rows, cols = row_num // row_interval, col_num // col_interval

    for i in range(rows):
        for j in range(cols):
            img_name = '%d_%d_%s' % (i, j, out_name)
            r_start, c_start = i * row_interval, j * col_interval
            r_end = (i + 1) * row_interval
            c_end = (j + 1) * col_interval
            # print([r_start, r_end, c_start, c_end])
            if len(img.shape) > 2:
                cv2.imwrite(out_folder + img_name,
                            img[r_start: r_end, c_start: c_end, :])
            else:
                cv2.imwrite(out_folder + img_name,
                            img[r_start: r_end, c_start: c_end])


def divide_img_by_interval_using_pil(channel1, channel2, channel3, out_folder, out_name, row_interval=1000, col_interval=1000):
    check_folder_exist(out_folder)
    row_num, col_num = channel1.shape[:2]
    rows, cols = row_num // row_interval, col_num // col_interval
    orig_img = np.stack((channel1, channel2, channel3), axis=2)
    for i in range(rows):
        for j in range(cols):
            img_name = '%d_%d_%s' % (i, j, out_name)
            r_start, c_start = i * row_interval, j * col_interval
            r_end = (i + 1) * row_interval
            c_end = (j + 1) * col_interval
            # print([r_start, r_end, c_start, c_end])
            image_path = out_folder + img_name
            temp_img = orig_img[r_start: r_end, c_start: c_end, :]
            image = Image.fromarray(temp_img)
            image.save(image_path)


def divide_grayscale_img_by_interval_using_pil(channel1, out_folder, out_name, row_interval=1000, col_interval=1000):
    check_folder_exist(out_folder)
    row_num, col_num = channel1.shape[:2]
    rows, cols = row_num // row_interval, col_num // col_interval
    orig_img = channel1
    for i in range(rows):
        for j in range(cols):
            img_name = '%d_%d_%s' % (i, j, out_name)
            r_start, c_start = i * row_interval, j * col_interval
            r_end = (i + 1) * row_interval
            c_end = (j + 1) * col_interval
            # print([r_start, r_end, c_start, c_end])
            image_path = out_folder + img_name
            temp_img = orig_img[r_start: r_end, c_start: c_end]
            image = Image.fromarray(temp_img)
            image.save(image_path)


def find_max_dims(files):
    max_r, max_c = 0, 0
    for f in files:
        f_splits = f.split('_')
        temp_r, temp_c = int(f_splits[0]), int(f_splits[1])
        if max_r < temp_r:
            max_r = temp_r
        if max_c < temp_c:
            max_c = temp_c
    return max_r, max_c


def check_label(cand_files, label):
    for c_f in cand_files:
        if label in c_f:
            return True
    return False


def find_cand_files_with_label(cand_files, label):
    res = []
    for c_f in cand_files:
        if label in c_f:
            res.append(c_f)
    return res


def merge_img(in_folder, sample_id, year, direction):
    check_folder = in_folder
    files = find_by_suffix(check_folder, 'png')
    cand_files = []
    for f in files:
        if re.search(str(year), f):
            if re.search(direction, f):
                cand_files.append(f)
    if check_label(cand_files, '_1_') and check_label(cand_files, '_h_'):
        cand_files = find_cand_files_with_label(cand_files, '_h_')
    start_date = re.search("_\d{8}", cand_files[0]).group()
    cand_files = [cand for cand in cand_files if start_date in cand]

    max_r, max_c = find_max_dims(cand_files)

    suffix_name = '_'.join(cand_files[0].split('_')[2:])

    r_row, g_row, b_row = [], [], []
    for i in range(max_r + 1):

        for j in range(max_c + 1):

            temp_img = cv2.imread(check_folder + '%d_%d_%s' %
                                  (i, j, suffix_name))

            if j == 0:
                temp_r, temp_g, temp_b = temp_img[:, :,
                                                  0], temp_img[:, :, 1], temp_img[:, :, 2]
            else:
                temp_r, temp_g, temp_b = np.hstack((temp_r, temp_img[:, :, 0])), \
                    np.hstack((temp_g, temp_img[:, :, 1])), \
                    np.hstack((temp_b, temp_img[:, :, 2]))

        r_row.append(temp_r)
        g_row.append(temp_g)
        b_row.append(temp_b)

    r_res, g_res, b_res = np.vstack(r_row), np.vstack(g_row), np.vstack(b_row)
    return np.stack((r_res, g_res, b_res), axis=2)


def get_split_meta(out_meta, r_start, r_end, c_start, c_end):
    orig_trans = out_meta.get('transform')
    gdal_trans = Affine.to_gdal(orig_trans)
    row_num = r_end - r_start
    col_num = c_end - c_start
    Xgeo = gdal_trans[0] + c_start * gdal_trans[1] + r_start * gdal_trans[2]
    Ygeo = gdal_trans[3] + c_start * gdal_trans[4] + r_start * gdal_trans[5]
    out_meta.update({
        # "driver": "GTiff",
        "height": row_num,
        "width": col_num,
        "transform": Affine(gdal_trans[1], gdal_trans[2], Xgeo, gdal_trans[4], gdal_trans[5], Ygeo)
    })
    return out_meta


def find_meta_by_id_year(pckl_folder, sample_id, year):
    check_folder = pckl_folder
    files = find_by_suffix(check_folder, 'pckl')
    cand_files = []
    for f in files:
        if re.search(str(int(year)), f):
            if re.search(str(int(sample_id)), f):
                cand_files.append(f)
    return cand_files


def find_meta_by_direction(cand_files, direction):
    for c in cand_files:
        if re.search(direction, c):
            return c
    return ''


def find_ymd_from_year(pckl_folder, sample_id, year):
    cand_files = find_meta_by_id_year(pckl_folder, sample_id, year)
    directions = ['sw', 'se', 'nw', 'ne']
    for direction in directions:
        target_file = find_meta_by_direction(cand_files, direction)
        if target_file:
            break
    return target_file.split('.')[0].split('_')[-1]


def get_merged_data_by_point(img_folder, pckl_folder, raster_save_folder, sample_id, year):
    temp_save_folder = f'./temp_{uuid.uuid4()}/'
    dir_list = ['sw', 'se', 'nw', 'ne']
    cand_files = find_meta_by_id_year(pckl_folder, str(sample_id), year)
    check_folder_exist(temp_save_folder)
    check_folder_exist(raster_save_folder)

    for direction in dir_list:

        temp_meta_name = find_meta_by_direction(cand_files, direction)

        if not temp_meta_name:
            continue
        else:
            temp_img = merge_img(img_folder, str(sample_id), year, direction)
            temp_meta = load_pickle(pckl_folder, temp_meta_name)
            raster_name = temp_meta_name.split('.')[0] + '.' + 'tif'
            saved_img = np.stack(
                (temp_img[:, :, 2], temp_img[:, :, 1], temp_img[:, :, 0]), axis=0)
            temp_meta.update({
                "driver": "GTiff",
                "height": temp_img.shape[0],
                "width": temp_img.shape[1],
            })
            save_new_raster(saved_img, temp_meta,
                            temp_save_folder, raster_name)

    file_tifs = find_by_suffix(temp_save_folder, 'tif')

    if file_tifs:
        actual_year = cand_files[0].split('.')[0].split('_')[-1][:4]
        final_save_name = str(sample_id) + '_' + str(actual_year) + '.' + 'tif'

        final_save_folder = raster_save_folder + "/" + str(sample_id) + '/'
        check_folder_exist(final_save_folder)
        if len(file_tifs) == 1:
            shutil.copy(temp_save_folder +
                        file_tifs[0], final_save_folder + final_save_name)
        else:
            with rasterio.open(temp_save_folder + file_tifs[0]) as src:
                meta = src.meta.copy()
            print("merge start")
            to_merge = [rasterio.open(temp_save_folder + f0)
                        for f0 in file_tifs]

            # The merge function returns a single array and the affine transform info
            arr, out_trans = merge(to_merge)

            meta.update({
                "driver": "GTiff",
                "height": arr.shape[1],
                "width": arr.shape[2],
                "transform": out_trans
            })

            check_folder_exist(final_save_folder)

            for file in to_merge:
                file.close()

            with rasterio.open(final_save_folder + final_save_name, "w", **meta) as dest:
                dest.write(arr)

    shutil.rmtree(temp_save_folder)


def extract_NAIP_folder(zip_folder, raster_folder, div_rgb_folder, div_infrared_folder, pckl_folder, txt_folder, sub_folder=None,
                        is_raster_del=True, is_infrared_record=True, is_txt_record=False):
    check_folder_exist(raster_folder)
    check_folder_exist(div_rgb_folder)
    check_folder_exist(pckl_folder)
    if is_infrared_record:
        check_folder_exist(div_infrared_folder)
    if is_txt_record:
        check_folder_exist(txt_folder)
    if sub_folder != None:
        zip_folder, raster_folder, div_rgb_folder, div_infrared_folder, txt_folder, pckl_folder = \
            zip_folder + sub_folder + '/', raster_folder + sub_folder + '/', div_rgb_folder + sub_folder + '/', \
            div_infrared_folder + sub_folder + '/', txt_folder + \
            sub_folder + '/', pckl_folder + sub_folder + '/'
        check_folder_exist(raster_folder)
        check_folder_exist(div_rgb_folder)
        check_folder_exist(pckl_folder)
        if is_infrared_record:
            check_folder_exist(div_infrared_folder)
        if is_txt_record:
            check_folder_exist(txt_folder)
    zip_files = find_by_suffix(zip_folder, 'ZIP')
    print_list = []
    print_list.append('LON,LAT,SAMPLE_ID,YEAR,FILE_NAME')
    for z in tqdm(zip_files):
        print('Now we are Processing: \n %s' % z)
        temp_str = ''
        try:
            archive = zipfile.ZipFile(zip_folder + z, 'r')
            file_front = z.split('.')[0]
            year = file_front.split('_')[-1][:4]
            sample_id = file_front.split('_')[1]
            check_id_year_exist(raster_folder, sample_id, year)
            raster_save_folder = raster_folder + \
                str(year) + '/' + str(sample_id) + '/'
            archive.extract(file_front + '.tif', raster_save_folder)
            check_id_year_exist(div_rgb_folder, sample_id, year)
            check_id_year_exist(pckl_folder, sample_id, year)
            check_id_year_exist(div_infrared_folder, sample_id, year)

            raster_file = file_front + '.tif'
            img_orig_folder = div_rgb_folder + \
                str(year) + '/' + str(sample_id) + '/'
            pckl_orig_folder = pckl_folder + \
                str(year) + '/' + str(sample_id) + '/'
            infrared_orig_folder = div_infrared_folder + \
                str(year) + '/' + str(sample_id) + '/'
            pckl_name = file_front + '.' + 'pckl'
            out_meta = extract_raster_meta(raster_save_folder, raster_file)
            save_pickle(out_meta, pckl_orig_folder, pckl_name)
            orig_img = get_img_from_raster(raster_save_folder, raster_file)
            divide_img_by_interval(orig_img, img_orig_folder, file_front +
                                   '.' + 'png', row_interval=1000, col_interval=1000)
            if is_infrared_record:
                if raster_file.split('_')[0] == 'm' or raster_file.split('_')[0] == 'M':
                    infrared_img = get_infrared_img_from_raster(
                        raster_save_folder, raster_file)
                    divide_grayscale_img_by_interval_using_pil(
                        infrared_img, infrared_orig_folder, file_front + '.png', row_interval=1000, col_interval=1000)

            Proj_str = str(out_meta.get('crs')).split('(')[-1].split(')')[0]
            inProj = Proj(init=Proj_str)
            outProj = Proj(init='epsg:4326')
            row_num, col_num = out_meta.get('height'), out_meta.get('width')

            check_row, check_col = row_num // 2, col_num // 2

            orig_trans = out_meta.get('transform')
            gdal_trans = Affine.to_gdal(orig_trans)

            check_x = gdal_trans[0] + check_col * \
                gdal_trans[1] + check_row * gdal_trans[2]
            check_y = gdal_trans[3] + check_col * \
                gdal_trans[4] + check_row * gdal_trans[5]

            check_lon, check_lat = transform(inProj, outProj, check_x, check_y)
            temp_str = '%f,%f,%s,%s,%s' % (
                check_lon, check_lat, sample_id, year, z)

            print_list.append(temp_str)

            time.sleep(0.1)
            if is_raster_del:
                shutil.rmtree(raster_save_folder)
        except:
            if not temp_str:
                file_front = z.split('.')[0]
                year = file_front.split('_')[-1][:4]
                sample_id = file_front.split('_')[1]
                temp_str = '%f,%f,%s,%s,%s' % (-999999, -
                                               999999, sample_id, year, z)
                print_list.append(temp_str)
            else:
                if not print_list:
                    print_list.append(temp_str)
                elif print_list[-1] != temp_str:
                    print_list.append(temp_str)
            continue
    if is_txt_record:
        with open(txt_folder + 'lon_lat_info.txt', 'w') as f:
            for p in print_list:
                print(p, file=f)


def merge_detect_folder(detect_root_folder, pickle_folder, final_raster_folder, print_log=True):
    subfolders = get_subfolders_with_digits(detect_root_folder)
    for sf in tqdm(subfolders):
        temp_detect_folder = os.path.join(detect_root_folder, sf)
        temp_subfolders = get_subfolders_with_digits(temp_detect_folder)
        temp_year = sf
        if print_log:
            print('We are now processing %s' % sf)
        for tsf in temp_subfolders:
            check_detect_folder = os.path.join(temp_detect_folder, tsf)
            temp_site = tsf
            check_pickle_folder = os.path.join(
                pickle_folder, temp_year, temp_site)
            get_merged_data_by_point(
                check_detect_folder, check_pickle_folder, final_raster_folder, temp_site, temp_year)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(
        description='Environment Settings', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-zp', '--zip_path', dest='zip_path', type=str,
                        default='/media/weiwang/easystore/NAIP/', help='Path to zip files')
    parser.add_argument('-rp', '--raster_path', dest='raster_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Raster/', help='Path to raster files')
    parser.add_argument('-imp', '--img_path', dest='img_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Divided_Img/', help='Path to divided imgs')
    parser.add_argument('-inp', '--infrared_path', dest='infrared_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Infrared/', help='Path to divided infrared imgs')
    parser.add_argument('-pp', '--pickle_path', dest='pickle_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Pckl_Folder/', help='Path to pickle files')
    parser.add_argument('-frp', '--final_raster_path', dest='final_raster_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Final_Raster_Folder/', help='Path to the merged raster files')
    parser.add_argument('-tp', '--txt_path', dest='txt_path', type=str,
                        default='/media/weiwang/easystore/NAIP/TXT_Folder/', help='Path to the coordinate files')
    parser.add_argument('-ird', '--if_raster_deleted', dest='if_raster_deleted', type=bool,
                        default=True, help='If deleted the raster file after extracting imgs and pckls')
    parser.add_argument('-dd', '--detected_img_path', dest='detected_img_path', type=str,
                        default='/media/weiwang/easystore/NAIP/Marked_Img/')
    parser.add_argument('-l', '--lake', dest='lake', type=str,
                        default='LakeZero', help='Which lake to process')
    parser.add_argument('-s', '--stage', dest='stage', type=int,
                        default=1, help='which stage to process, 1 is extraction and division, 2 is merging')

    args = parser.parse_args()
    stage = args.stage
    if stage == 1:
        extract_NAIP_folder(args.zip_path, args.raster_path, args.img_path, args.infrared_path, args.pickle_path, args.txt_path, sub_folder=args.lake,
                            is_raster_del=True, is_infrared_record=True, is_txt_record=False)
    if stage == 2:
        merge_detect_folder(args.detected_img_path, args.pickle_path,
                            args.final_raster_path, print_log=True)
