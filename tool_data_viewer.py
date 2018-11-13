"""
Author: Yifei Wang
Date: 2018-11-12
Description: This script provide feature that view mask and image together, give more directly view of data
"""

import os
import cv2
import numpy as np

# the directory where to store the image and mask pair
DIR_WORKING_DIR = r"H:\Maxieye_Data\RAIN\Data1_920\jpg_b"

# the directory where to store the valid image and mask pair
DIR_VALID_DIR = r"H:\Maxieye_Data\RAIN\Data1_920\valid_no_drop"


def _find_valid_file_pair(working_dir):
    all_file_list = os.listdir(working_dir)
    valid_file_list = list()

    for file in all_file_list:
        is_mask_file = file.endswith(".jpg")
        if is_mask_file:
            valid_file_list.append(file)

    return valid_file_list


def _view_mask_and_image_file(working_dir, valid_dir, valid_file_list):

    for idx, valid_file in enumerate(valid_file_list):
        image_file_path = os.path.join(working_dir, valid_file)

        image_file_data = cv2.imread(image_file_path)

        image_file_data = cv2.resize(image_file_data, (320, 160))

        key = -1

        while key != 32 and key != 115 and key != 83:  # space, s

            display_data = image_file_data

            cv2.imshow("view_data", display_data)
            key = cv2.waitKey(-1)

        if key == 115 or key == 83:
            save_image_file_path = os.path.join(valid_dir, valid_file)
            os.rename(image_file_path, save_image_file_path)

        print("Progress %d/%d" % (idx+1, len(valid_file_list)))


valid_list = _find_valid_file_pair(DIR_WORKING_DIR)
_view_mask_and_image_file(DIR_WORKING_DIR, DIR_VALID_DIR, valid_list)
