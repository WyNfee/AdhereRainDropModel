"""
Author: Yifei Wang
Date: 2018-10-31
Description: This script will generate 3D mesh (including normal) according to sample points
"""

import numpy as np
import random
import matplotlib.pyplot as plt
import bezier_curve as bezier
from mpl_toolkits.mplot3d import Axes3D

# the out shape x sample point file
FILE_OUT_SHAPE_X = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\sampled\out_shape_x.npy"

# the out shape y sample point file
FILE_OUT_SHAPE_Y = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\sampled\out_shape_y.npy"

# the out shape y sample point file
FILE_PEAK_SHAPE = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\sampled\peak_shape.npy"

# the out shape y sample point file
FILE_PEAK_HEIGHT = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\sampled\peak_height.npy"


def _random_generate_pick_index(data):
    """
    randomly generate a pick index given sample set of data
    :param data: the sample set of data
    :return: the selected index
    """
    data_count = data.shape[0]
    select_idx = int(random.uniform(0, data_count))
    return select_idx


def _abstract_sample_data(x_data, y_data, p_height_data, p_shape_data,
                          long, width, height,
                          shape_offset=0.2,
                          enable_debugging=False):
    """
    abstract the sample data and build 3d basic model
    :param x_data: the out shape x data
    :param y_data: the out shape y data
    :param p_height_data: the height data of peak
    :param p_shape_data: the shape data of the peak
    :param long: the actual length on pixel of the data to generate
    :param width: the actual width on pixel of the data to generate
    :param height: the actual height on pixel of the data to generate
    :param shape_offset: the shape offset (of percentage to long)
    :param enable_debugging: whether enable the data to preview
    :return:
    """

    sample_strength = p_height_data.shape[0]
    abstracted_data = np.zeros([sample_strength, sample_strength, 3])

    for idx in range(sample_strength):
        if idx != 0:
            ox1 = x_data[idx] * width
            oy1 = y_data[idx] * long
            oy2 = y_data[-1 - idx] * long

            ph = p_height_data[idx] * height
            ps = p_shape_data[idx] * long * shape_offset

            start_point = (oy1, 0)
            end_point = (oy2, 0)
            middle_point = (ps, ph)

            control_points = (start_point, middle_point, end_point)
            axis_1, axis_2 = bezier.sample_bezier_curve(control_points, sample_strength)

            axis_0 = np.ones([sample_strength]) * ox1

            abstracted_data[idx, :, 0] = axis_0
            abstracted_data[idx, :, 1] = axis_1
            abstracted_data[idx, :, 2] = axis_2

    if enable_debugging is True:
        fig = plt.figure()
        ax = fig.gca(projection='3d')

        ax.set_title("rain drop model Model")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')

        shape_height = abstracted_data.shape[0]

        for h_idx in range(shape_height):
            if h_idx % 20 == 0:
                ax.plot(
                    abstracted_data[h_idx, :, 0],
                    abstracted_data[h_idx, :, 1],
                    abstracted_data[h_idx, :, 2],
                    'b-'
                )

        plt.show()


def load_sample_data(shape_x, shape_y, peak_height, peak_shape):
    """
    load the data from numpy saved object
    :param shape_x: the save file name of shape x
    :param shape_y: the save file name of shape y
    :param peak_height: the save file name pf peak height
    :param peak_shape: the save file name of peak shape
    :return: the loaded data of each save file
    """
    x_data = np.load(shape_x)
    y_data = np.load(shape_y)
    p_height_data = np.load(peak_height)
    p_shape_data = np.load(peak_shape)

    return x_data, y_data, p_height_data, p_shape_data


data_x, data_y, data_ph, data_ps = load_sample_data(FILE_OUT_SHAPE_X, FILE_OUT_SHAPE_Y, FILE_PEAK_HEIGHT, FILE_PEAK_SHAPE)
selected_index = _random_generate_pick_index(data_x)
_abstract_sample_data(data_x[selected_index], data_y[selected_index], data_ph[selected_index], data_ps[selected_index],
                      100, 80, 12, 0.1,
                      enable_debugging=True)
print("complete!")
