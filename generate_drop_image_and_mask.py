"""
Author: Yifei Wang
Date: 2018-11-01
Description: This script will be used to generate the water drop on image
"""

import cv2
import os
import draw_drop_on_image as drop_draw

# the directory where to store the
DIR_STORE_IMAGE_TO_OPERATE = ""


all_list_files = os.listdir(DIR_STORE_IMAGE_TO_OPERATE)

image_file_list = list()

for file in all_list_files:
    if file.endswith(".bmp") is True or file.endswith(".jpg") is True:
        image_file_list.append(os.path.join(DIR_STORE_IMAGE_TO_OPERATE, file))


for idx, file in enumerate(image_file_list):
    drop_image, drop_mask = drop_draw.generate_image_with_water_drop(file)

    cv2.imwrite("%d.jpg" % idx, drop_image)
    cv2.imwrite("%d_mask.jpg" % idx, drop_mask)

    print("Processed %d image" % idx+1)
