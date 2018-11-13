"""
Author: Yifei Wang
Date: 2018-11-01
Description: Write the generated drop data to tfrecord
"""

import os
import cv2
import numpy as np
import tensorflow as tf
import random


# the directory of drop data to read
DROP_DATA_DIR = r"H:\Maxieye_Data\RAIN\Data1_920\generated"

# the directory of non drop adhere data to read
NON_DROP_DATA_DIR = r"H:\Maxieye_Data\RAIN\Data1_920\jpg_b"

# the output file path
OUTPUT_TFRECORD = r"D:\Data\TFTrain\AdhereRainDrop\Record\drop.tfrecords"

# whether enable debugging
ENABLE_DEBUGGING = True


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


def read_the_non_drop_data(non_drop_data_directory):
    file_list = os.listdir(non_drop_data_directory)
    non_drop_data_list = list()

    for file in file_list:
        if file.endswith(".jpg"):
            file_path_to_store = os.path.join(non_drop_data_directory, file.split(".")[0])
            non_drop_data_list.append(file_path_to_store)

    return non_drop_data_list


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


def preprocess_non_drop_data(non_drop_data):
    image_path = non_drop_data + '.jpg'

    image_data = cv2.imread(image_path)
    image_data = image_data[77:-3, :]
    mask_data = np.zeros_like(image_data)

    input_image_data = cv2.resize(image_data, (320, 160))
    input_mask_data = cv2.resize(mask_data, (40, 20))

    if ENABLE_DEBUGGING:
        cv2.imshow("input_image", input_image_data)
        cv2.imshow("input_mask", input_mask_data)
        cv2.waitKey(-1)

    return input_image_data, input_mask_data


def generate_tfrecords(drop_data_dir, non_drop_data_dir, output_file):
    drop_data_path_list = read_the_drop_data(drop_data_dir)
    non_drop_data_list = read_the_non_drop_data(non_drop_data_dir)

    data_path_list = list()

    for dp in drop_data_path_list:
        data_path_list.append([dp, True])

    for dp in non_drop_data_list:
        data_path_list.append([dp, False])

    # shuffle twice to make sure the data inside is randomized
    random.shuffle(data_path_list)
    random.shuffle(data_path_list)

    writer = tf.python_io.TFRecordWriter(output_file)

    example_amount = len(data_path_list)

    for idx, data in enumerate(data_path_list):
        is_drop_data = data[1]
        data_path = data[0]
        if is_drop_data is True:
            image_data, mask_data = preprocess_drop_data(data_path)
        else:
            image_data, mask_data = preprocess_non_drop_data(data_path)

        example = tf.train.Example(features=tf.train.Features(
            feature={
                'image': _bytes_feature(image_data.tostring()),
                'value': _bytes_feature(mask_data.tostring()),
            }
        ))
        writer.write(example.SerializeToString())
        print('Progress %d/%d' % (idx + 1, example_amount))

    writer.close()


generate_tfrecords(DROP_DATA_DIR, NON_DROP_DATA_DIR, OUTPUT_TFRECORD)
