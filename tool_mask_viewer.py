"""
Author: Yifei Wang
Date: 2018-11-12
Description: This script provide feature that view mask and image together, give more directly view of data
"""

import os
import cv2
import numpy as np

# the directory where to store the image and mask pair
DIR_WORKING_DIR = r"H:\Maxieye_Data\RAIN\Data1_920\generated"


def _find_valid_file_pair(working_dir):
    all_file_list = os.listdir(working_dir)
    valid_file_list = list()

    for file in all_file_list:
        is_mask_file = file.endswith("_mask.jpg")
        file_pair_name = file[:-len("_mask.jpg")]
        image_file_name = file_pair_name + ".jpg"
        image_file_path = os.path.join(working_dir, image_file_name)
        if is_mask_file and os.path.exists(image_file_path):
            valid_file_list.append(file_pair_name)

    return valid_file_list


def _view_mask_and_image_file(working_dir, valid_file_list):
    display_mode = 1

    for idx, valid_file in enumerate(valid_file_list):
        image_file_path = os.path.join(working_dir, valid_file+".jpg")
        mask_file_path = os.path.join(working_dir, valid_file+"_mask.jpg")

        image_file_data = cv2.imread(image_file_path)
        mask_file_data = cv2.imread(mask_file_path)

        image_file_data = cv2.resize(image_file_data, (320, 160))
        mask_file_data = cv2.resize(mask_file_data, (320, 160))

        display_data = image_file_data + mask_file_data * [128, 0, 128]
        display_data = np.clip(display_data, 0, 255)
        display_data = display_data.astype(np.uint8)

        key = -1

        while key != 32:  # space

            if key == 90 or key == 122:  # press z
                display_mode = 1
            elif key == 88 or key == 120:  # press x
                display_mode = 2
            elif key == 67 or key == 99:  # press c
                display_mode = 3

            if display_mode == 1:
                display_data = image_file_data + mask_file_data * [128, 0, 128]
                display_data = np.clip(display_data, 0, 255)
                display_data = display_data.astype(np.uint8)
            elif display_mode == 2:
                display_data = image_file_data
            elif display_mode == 3:
                display_data = mask_file_data

            cv2.imshow("view_data", display_data)
            key = cv2.waitKey(-1)

        print("Progress %d/%d" % (idx+1, len(valid_file_list)))


valid_list = _find_valid_file_pair(DIR_WORKING_DIR)
_view_mask_and_image_file(DIR_WORKING_DIR, valid_list)
