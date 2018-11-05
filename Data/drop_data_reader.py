"""
Author: Yifei Wang
Date: 2018-11-01
Description: This script will read the drop data from tfrecords
"""

import tensorflow as tf


def _read_and_decode(file_name_queue):
    reader = tf.TFRecordReader()
    _, serialized_example = reader.read(file_name_queue)

    features = tf.parse_single_example(
        serialized_example,
        features={
            'image': tf.FixedLenFeature([], tf.string),
            'value': tf.FixedLenFeature([], tf.string),
        }
    )

    # first image data

    image_data = tf.decode_raw(features['image'], tf.uint8)
    image_data = tf.reshape(image_data, [160, 320, 3])

    summary_image_data = tf.expand_dims(image_data, axis=0)
    tf.summary.image('InputImage', summary_image_data)

    image_data = tf.cast(image_data, dtype=tf.float32)
    image_data = image_data - 128.0

    # then mask data

    mask_data = tf.decode_raw(features['value'], tf.uint8)
    mask_data = tf.reshape(mask_data, [20, 40, 3])
    mask_data = mask_data[:, :, 0]

    summary_mask_data = tf.expand_dims(mask_data, axis=-1)
    summary_mask_data = tf.expand_dims(summary_mask_data, axis=0)
    tf.summary.image('InputMask', summary_mask_data)

    # only value > 128 will be consider as positive sample with label 1
    # otherwise, after resize, some pixel may be < 128, we will treat them as negative
    # though hard cutting off...
    mask_data = tf.greater(mask_data, 128)
    mask_data = tf.cast(mask_data, dtype=tf.int32)

    return image_data, mask_data


def create_input(record_file, batch_size, exe_epoch=None):
    # load the dataset
    file_name_queue = tf.train.string_input_producer([record_file], num_epochs=exe_epoch)

    image, label = _read_and_decode(file_name_queue)

    images, labels = tf.train.batch([image, label], batch_size=batch_size)

    return images, labels
