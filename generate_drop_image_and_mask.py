"""
Author: Yifei Wang
Date: 2018-11-01
Description: This script will be used to generate the water drop on image
"""

import cv2
import os
import random
import numpy as np
import Drop.draw_drop_on_image as drop_draw

# the directory where to store the
DIR_STORE_IMAGE_TO_OPERATE = r"D:\Data\TFTrain\AdhereRainDrop\Data\valid_no_drop"

# the directory where to save the output file
DIR_SAVE_IMAGE_GENERATED = r"D:\Data\TFTrain\AdhereRainDrop\Data\generated_drop"

# whether enable debugging for not
ENABLE_DEBUGGING = False

# the image list to be duplicated for generate fake image drop on windshield
IMAGE_LIST_DUPLICATE_MULTIPLIER = 1.

all_list_files = os.listdir(DIR_STORE_IMAGE_TO_OPERATE)

image_file_list = list()

for file in all_list_files:
    if file.endswith(".bmp") is True or file.endswith(".jpg") is True:
        image_file_list.append(os.path.join(DIR_STORE_IMAGE_TO_OPERATE, file))

image_file_list = image_file_list * IMAGE_LIST_DUPLICATE_MULTIPLIER
random.shuffle(image_file_list)
random.shuffle(image_file_list)

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
