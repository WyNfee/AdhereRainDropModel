"""
Author: Yifei Wang
Date: 2018-11-12
Description: This script will generate the mask (json) file from labeled drop image
"""

import os
import json
import cv2
import numpy as np

# the directory where to find the json and image file pair
DIR_LABELED_DATA = r"D:\Maxieye_Data\RAIN\Data1_920\jpg_a"

# the directory where to store the json file and image file
DIR_SAVE_IMAGE_GENERATED = r"D:\Maxieye_Data\RAIN\Data1_920\generated"

# the directory where to store invalid files
DIR_INVALID_FILES = r"D:\Maxieye_Data\RAIN\Data1_920\error"

# whether enable debugging plot
ENABLE_DEBUGGING = False


def _find_valid_file_pair(working_dir):
    """
    Looking for directory and find a valid pair (json + jpg) both exist
    :param working_dir: the working directory to find data
    :return: a list contains file safe name
    """
    all_files_in_list = os.listdir(working_dir)

    valid_file_list = list()

    for file in all_files_in_list:
        is_json_file = file.endswith(".json")
        file_safe_name = file.split(".")[0]
        paired_file = file_safe_name + ".jpg"
        paired_file_path = os.path.join(working_dir, paired_file)
        if os.path.exists(paired_file_path) and is_json_file is True:
            valid_file_list.append(file_safe_name)

    print("collecting valid file: %d" % len(valid_file_list))
    return valid_file_list


def generate_mask_and_convert_image(working_dir, save_dir, image_safe_name, enable_debug=False):

    image_file_path = os.path.join(working_dir, image_safe_name + ".jpg")
    json_file_path = os.path.join(working_dir, image_safe_name + ".json")

    json_file_obj = open(json_file_path, "r")
    json_data = json.load(json_file_obj)
    json_file_obj.close()

    points_list = list()

    obj_list = json_data["ObjList"]

    if obj_list is None:
        print("[Error] something wrong at file %s" % image_safe_name)
        image_error_image = os.path.join(DIR_INVALID_FILES, image_safe_name + ".jpg")
        json_error_image = os.path.join(DIR_INVALID_FILES, image_safe_name + ".json")
        os.rename(image_file_path, image_error_image)
        os.rename(json_file_path, json_error_image)
        return

    for obj in obj_list:
        points = obj["regionArr"][0]["region_borderPts"]
        points_list.append(points)

    image_data = cv2.imread(image_file_path)
    image_height, image_width, _ = image_data.shape
    mask_base_data = np.zeros([image_height, image_width], dtype=np.float32)

    if enable_debug is True:
        cv2.imshow("dbg_mask", mask_base_data)
        cv2.imshow("dbg_image", image_data)
        cv2.waitKey(-1)

    for points in points_list:
        points_arr = np.asarray(points, dtype=np.int32)
        mask_image = cv2.fillPoly(mask_base_data, np.array([points_arr], dtype=np.int32), 1)

        if enable_debug is True:
            cv2.imshow("dbg_mask", mask_image)
            cv2.waitKey(-1)

    if enable_debug is True:
        cv2.destroyAllWindows()

    mask_image = mask_image[77:-3, :]
    image_data = image_data[77:-3, :]

    drop_mask = mask_image * 255
    drop_mask = drop_mask.astype(np.uint8)
    drop_mask = np.stack([drop_mask, drop_mask, drop_mask], axis=-1)

    image_file_path = os.path.join(save_dir, "%s.jpg" % image_safe_name)
    mask_file_path = os.path.join(save_dir, "%s_mask.jpg" % image_safe_name)

    cv2.imwrite(image_file_path, image_data)
    cv2.imwrite(mask_file_path, drop_mask)


valid_file_names = _find_valid_file_pair(DIR_LABELED_DATA)
all_data_amount = len(valid_file_names)
for idx, valid_file in enumerate(valid_file_names):
    generate_mask_and_convert_image(DIR_LABELED_DATA, DIR_SAVE_IMAGE_GENERATED, valid_file)
    print("Progress %d/%d" % (idx+1, all_data_amount))


# Debugging Function
# generate_mask_and_convert_image(
#     r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\real_drop",
#     r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\drop",
#     "20180920_0048193678_001", ENABLE_DEBUGGING)
