"""
Author: Yifei Wang
Date: 2018-11-01
Description: The script will demo drop net result
"""

import os
import cv2
import numpy as np
import tensorflow as tf
from Model import drop_net as net
import copy

# the file path of srpn weights
WEIGHT_FILE = r"D:\Data\TFTrain\AdhereRainDrop\Save\test"

# the directory where store the demo image
DIR_DEMO_IMAGE = r"D:\Data\TFTrain\AdhereRainDrop\T3"

# selection threshold of srpn network
SELECTION_THRESHOLD = 0.6

# CPU only mode
CPU_ONLY_MODE = False


def drop_net_detection(image_data):
    with tf.Graph().as_default():
        gpu_options = tf.GPUOptions(allow_growth=True)
        if CPU_ONLY_MODE:
            config = tf.ConfigProto(log_device_placement=False, gpu_options=gpu_options, device_count={'GPU': 0})
        else:
            config = tf.ConfigProto(log_device_placement=False, gpu_options=gpu_options)
        session = tf.Session(config=config)

        input_tensor = tf.placeholder(tf.uint8, shape=(None, None, 3))
        image_cast = tf.cast(input_tensor, dtype=tf.float32)
        image_pre = image_cast - 128.0
        input_image = tf.expand_dims(image_pre, 0)

        logits = net.get_net(input_image, is_training=False)

        predictions = tf.nn.softmax(logits)
        selection_map = tf.greater(predictions[:, :, :, 1], SELECTION_THRESHOLD)
        selection_map = tf.expand_dims(tf.cast(selection_map, dtype=tf.float32), axis=-1)

        filter_kernel = tf.ones((1, 1), dtype=tf.float32)
        filter_kernel = tf.pad(filter_kernel, [[1, 1], [1, 1]], mode="CONSTANT", constant_values=1)
        filter_kernel = tf.pad(filter_kernel, [[1, 1], [1, 1]], mode="CONSTANT", constant_values=0)
        filter_kernel = filter_kernel / 8.
        filter_kernel = tf.expand_dims(filter_kernel, axis=-1)
        filter_kernel = tf.expand_dims(filter_kernel, axis=-1)
        filtered_map = tf.nn.conv2d(selection_map, filter_kernel, strides=[1, 1, 1, 1], padding="SAME")
        filtered_map = tf.greater_equal(filtered_map, 0.5)

        selection = tf.cast(filtered_map, dtype=tf.uint8)

        session.run(tf.global_variables_initializer())
        saver = tf.train.Saver()
        saver.restore(session, WEIGHT_FILE)

        select_drop_map = session.run(selection, feed_dict={input_tensor: image_data})

        return select_drop_map


def main():
    if os.path.exists(DIR_DEMO_IMAGE) is False:
        raise ValueError("demo image directory does not exist, quit")

    all_image_file_list = os.listdir(DIR_DEMO_IMAGE)

    for i_idx, image_file in enumerate(all_image_file_list):
        image_file_name = os.path.join(DIR_DEMO_IMAGE, image_file)
        image_data = cv2.imread(image_file_name)

        if image_data is None:
            continue

        image_data = image_data[77:-3, :]
        orig_image = copy.deepcopy(image_data)
        image_data = cv2.resize(image_data, (320, 160))

        drop_map = drop_net_detection(image_data)

        drop_map_data = drop_map[0, :, :] * 255
        drop_map_data = drop_map_data.astype(np.uint8)

        resize_drop_map = cv2.resize(drop_map_data, (320, 160))

        cv2.imshow('orig_image', orig_image)
        cv2.imshow('drop_data', resize_drop_map)
        cv2.waitKey(-1)


main()
