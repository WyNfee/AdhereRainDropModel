"""
Author: Yifei Wang
Date: 2018-10-31
Description: This script will generate 3D mesh (including normal) according to sample points
"""

import numpy as np
import random
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from Drop import bezier_curve as bezier

# the out shape x sample point file
FILE_OUT_SHAPE_X = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\saved\sampled\out_shape_x.npy"

# the out shape y sample point file
FILE_OUT_SHAPE_Y = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\saved\sampled\out_shape_y.npy"

# the out shape y sample point file
FILE_PEAK_SHAPE = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\saved\sampled\peak_shape.npy"

# the out shape y sample point file
FILE_PEAK_HEIGHT = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Raw\saved\sampled\peak_height.npy"


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
    :return: the model mesh
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

    # transform the mesh data along axis, make all data start from 0
    min_x = np.min(abstracted_data[:, :, 0])
    min_y = np.min(abstracted_data[:, :, 1])
    min_z = np.min(abstracted_data[:, :, 2])

    abstracted_data[:, :, 0] = abstracted_data[:, :, 0] - min_x
    abstracted_data[:, :, 1] = abstracted_data[:, :, 1] - min_y
    abstracted_data[:, :, 2] = abstracted_data[:, :, 2] - min_z

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

    return abstracted_data


def _normalize_vector(input_vector):
    """
    normalize an input vector to a module of 1
    :param input_vector: the vector
    :return: the normalized vector
    """
    norm = np.linalg.norm(input_vector)
    norm_vec = input_vector / (norm + 1e-6)
    return norm_vec


def _compute_normal(mesh_data, current_h_idx, current_w_idx):
    """
    compute the normal of a mesh point, by sample its adjacent other points
    :param mesh_data: mesh data to work on
    :param current_h_idx: current mesh index on y axis
    :param current_w_idx: current mesh index on x axis (assuming z is for height)
    :return: the normal compute out
    """
    # get the point from upper, bottom, left and right
    current_pt = mesh_data[current_h_idx][current_w_idx]
    left_pt = mesh_data[current_h_idx][current_w_idx - 1]
    right_pt = mesh_data[current_h_idx][current_w_idx + 1]
    bottom_pt = mesh_data[current_h_idx + 1][current_w_idx]
    up_pt = mesh_data[current_h_idx - 1][current_w_idx]

    # compute the vector to these points
    left_vector = current_pt - left_pt
    right_vector = right_pt - current_pt
    up_vector = current_pt - up_pt
    bottom_vector = bottom_pt - current_pt

    # compute the normal using cross product
    left_up_normal = np.cross(left_vector, up_vector)
    left_bottom_normal = np.cross(left_vector, bottom_vector)
    right_up_normal = np.cross(right_vector, up_vector)
    right_bottom_normal = np.cross(right_vector, bottom_vector)

    # do normalize
    left_up_normal = _normalize_vector(left_up_normal)
    left_bottom_normal = _normalize_vector(left_bottom_normal)
    right_up_normal = _normalize_vector(right_up_normal)
    right_bottom_normal = _normalize_vector(right_bottom_normal)

    # add these normal at four direction and average, more precise on normal
    final_normal = (left_up_normal + left_bottom_normal + right_up_normal + right_bottom_normal) / 4

    # normalize the vector again
    final_normal = _normalize_vector(final_normal)

    return final_normal


def compute_normal_based_on_mesh(mesh, enable_debugging=False):
    """
    compute the normal based on the mesh created
    :param mesh: the mesh to work on
    :param enable_debugging: whether enable debugging plot
    :return:
    """
    height = mesh.shape[0]
    width = mesh.shape[1]

    normal_data = np.zeros_like(mesh)

    for h_idx in range(height):
        for w_idx in range(width):
            # the boundary item will not use to compute normal
            if 0 < h_idx < height-1 and 0 < w_idx < width-1:
                norm = _compute_normal(mesh, h_idx, w_idx)
                normal_data[h_idx][w_idx] = norm

    if enable_debugging is True:
        # NOTE: the magic number is arbitrary set here, just for debug purpose
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        ax.set_title("rain drop Mesh - 3D")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')
        ax.set_xlim(0, 100)
        ax.set_ylim(-50, 50)
        ax.set_zlim(0, 100)
        ax.plot_wireframe(mesh[:, :, 0], mesh[:, :, 1], mesh[:, :, 2], color='b')
        for m_h_idx in range(height):
            for m_w_idx in range(width):
                if m_h_idx % 100 == 0 and m_w_idx % 100 == 0:
                    normal = normal_data[m_h_idx][m_w_idx] * 5
                    point = mesh[m_h_idx, m_w_idx]
                    ax.quiver(point[0], point[1], point[2], normal[0], normal[1], normal[2], color='r')
        plt.show()

    return normal_data


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


"""
data_x, data_y, data_ph, data_ps = load_sample_data(FILE_OUT_SHAPE_X, FILE_OUT_SHAPE_Y, FILE_PEAK_HEIGHT, FILE_PEAK_SHAPE)
selected_index = _random_generate_pick_index(data_x)
model_data = _abstract_sample_data(data_x[selected_index], data_y[selected_index], data_ph[selected_index], data_ps[selected_index],
                                   40, 80, 12, 0.1,
                                   enable_debugging=True)
# this operation is really time consuming!!!
normal_data = compute_normal_based_on_mesh(model_data, enable_debugging=True)
print("complete!")
"""
