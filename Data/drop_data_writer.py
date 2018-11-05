"""
Author: Yifei Wang
Date: 2018-11-01
Description: Write the generated drop data to tfrecord
"""

import os
import cv2
import numpy as np
import tensorflow as tf


# the directory of drop data to read
DROP_DATA_DIR = r"D:\Data\TFTrain\AdhereRainDrop\Drop"

# the output file path
OUTPUT_TFRECORD = r"D:\Data\TFTrain\AdhereRainDrop\Record\drop.tfrecords"

# whether enable debugging
ENABLE_DEBUGGING = False


def _bytes_feature(value):
    return tf.train.Feature(bytes_list=tf.train.BytesList(value=[value]))


def read_the_drop_data(drop_data_directory):
    file_list = os.listdir(drop_data_directory)
    drop_data_list = list()

    for file in file_list:
        is_mask_data = file.find("_mask") != 0
        has_base_data = os.path.exists(os.path.join(drop_data_directory, file.split(".")[0].split("_")[0] + ".jpg"))
        if is_mask_data is True and has_base_data is True:
            file_path_to_store = os.path.join(drop_data_directory, file.split(".")[0].split("_")[0])
            drop_data_list.append(file_path_to_store)

    return drop_data_list


def preprocess_drop_data(drop_data):
    image_path = drop_data + '.jpg'
    mask_path = drop_data + '_mask.jpg'

    image_data = cv2.imread(image_path)
    mask_data = cv2.imread(mask_path)

    input_image_data = cv2.resize(image_data, (320, 160))
    input_mask_data = cv2.resize(mask_data, (40, 20))

    if ENABLE_DEBUGGING:
        cv2.imshow("input_image", input_image_data)
        cv2.imshow("input_mask", input_mask_data)
        cv2.waitKey(-1)

    return input_image_data, input_mask_data


def generate_tfrecords(drop_data_dir, output_file):
    data_path_list = read_the_drop_data(drop_data_dir)
    writer = tf.python_io.TFRecordWriter(output_file)

    example_amount = len(data_path_list)

    for idx, data_path in enumerate(data_path_list):
        image_data, mask_data = preprocess_drop_data(data_path)
        example = tf.train.Example(features=tf.train.Features(
            feature={
                'image': _bytes_feature(image_data.tostring()),
                'value': _bytes_feature(mask_data.tostring()),
            }
        ))
        writer.write(example.SerializeToString())
        print('Progress %d/%d' % (idx + 1, example_amount))

    writer.close()


generate_tfrecords(DROP_DATA_DIR, OUTPUT_TFRECORD)
