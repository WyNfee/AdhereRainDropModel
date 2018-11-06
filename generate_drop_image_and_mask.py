"""
Author: Yifei Wang
Date: 2018-11-01
Description: This script will be used to generate the water drop on image
"""

import cv2
import os
import numpy as np
import Drop.draw_drop_on_image as drop_draw

# the directory where to store the
DIR_STORE_IMAGE_TO_OPERATE = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\image"

# the directory where to save the output file
DIR_SAVE_IMAGE_GENERATED = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\drop"

ENABLE_DEBUGGING = True

all_list_files = os.listdir(DIR_STORE_IMAGE_TO_OPERATE)

image_file_list = list()

for file in all_list_files:
    if file.endswith(".bmp") is True or file.endswith(".jpg") is True:
        image_file_list.append(os.path.join(DIR_STORE_IMAGE_TO_OPERATE, file))


for idx, file in enumerate(image_file_list):
    drop_image, drop_mask = drop_draw.generate_image_with_water_drop(file)

    drop_mask = drop_mask * 255
    drop_mask = drop_mask.astype(np.uint8)
    drop_mask = np.stack([drop_mask, drop_mask, drop_mask], axis=-1)

    image_file_path = os.path.join(DIR_SAVE_IMAGE_GENERATED, "%d.jpg" % idx)
    mask_file_path = os.path.join(DIR_SAVE_IMAGE_GENERATED, "%d_mask.jpg" % idx)

    cv2.imwrite(image_file_path, drop_image)
    cv2.imwrite(mask_file_path, drop_mask)

    if ENABLE_DEBUGGING is True:
        cv2.imshow("IMAGE", drop_image)
        cv2.imshow("MASK", drop_mask)
        cv2.waitKey(-1)

    print("Processed %d image" % (idx + 1))
