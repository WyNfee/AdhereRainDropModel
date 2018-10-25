"""
Author: Yifei Wang
Date: 2018-10-24
Description: This script will generate a water drop model
"""
import numpy as np

import met_file_reader as reader
import bezier_curve as bezier

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D


def _get_curve_line(control_points):
    axis_1_points, axis_2_points = bezier.generate_bezier_curve(control_points)
    return axis_1_points, axis_2_points


def read_drop_data(data_file, length=10, width=10, height=8, peak_shape_offset=0.2, line_step=0.001, enable_debug=False):
    """
    read the drop data from file. and use parameters to build a model of rain drop data
    :param data_file: the data file to work on
    :param length: the length of a water drop (y axis)
    :param width: the width of a water drop (x axis)
    :param height: the height of a water drop (z axis)
    :param peak_shape_offset: the offset on y axis for peak location. ideally, [-1, 1]
    :param line_step: the precision of rain drop sampling
    :param enable_debug: whether enable debug plot
    :return:
    sxs: outline of rain drop (x, y coordinates) x coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    sys: outline of rain drop (x, y coordinates), y coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    psys: the peak shape projected on y axis, should be 2 times of base sampling that is 2 * 1 / line_step;
    pzs: the peak height of peak, on z axis, should be 2 times of base sampling, that is 2 * 1 / line_step
    lr: long range, the size of model, from y axis
    wr: width range, the size of model, from x axis
    hr: height range, the size of model, from z axis
    """
    h_curve_points, h_peak_points, shape_curve_points = reader.generate_curve_list(data_file, line_step)

    sxs = [p[0] * width for p in shape_curve_points]
    sys = [p[1] * length for p in shape_curve_points]
    psys = [p[1] * length * peak_shape_offset for p in h_curve_points]
    pzs = [p[1] * height for p in h_peak_points]

    min_x = min(sxs)
    min_y = min(sys)

    sxs = [x - min_x for x in sxs]
    sys = [y - min_y for y in sys]
    psys = [y - min_y for y in psys]

    if enable_debug is True:
        half_shape_length = int(len(sxs)/2)
        plt.plot(sxs, sys, label='shape')
        plt.plot(sxs[:half_shape_length], sys[:half_shape_length], label='half_shape')
        plt.plot(sxs[:half_shape_length], psys, label='peak_shape')
        plt.plot(sxs[:half_shape_length], pzs, label='peak_height')

    return sxs, sys, psys, pzs


def generate_3d_model(sxs, sys, psys, pzs, enable_debug=False):
    """
    use input parameters to generate 3d model of rain drop
    :param sxs: outline of rain drop (x, y coordinates) x coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    :param sys: outline of rain drop (x, y coordinates), y coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    :param psys: the peak shape projected on y axis, should be 2 times of base sampling that is 2 * 1 / line_step;
    :param pzs: the peak height of peak, on z axis, should be 2 times of base sampling, that is 2 * 1 / line_step
    :param enable_debug: whether enable debug plot
    :return:
    model_data: a list of lists, length is the sampling over x axis.
    each element of list, contains 3 other lists, each represent the list of points on x axis, y axis, and z axis,
    each list contains 1000 elements by default
    xmax: the max value of model data, from x axis
    ymax: the max value of model data, from y axis
    """

    model_data = list()

    x_axis_iter_length = int(len(sxs) / 2)
    for idx in range(x_axis_iter_length):
        sx1 = sxs[idx]
        sy1 = sys[idx]
        sy2 = sys[-idx]
        psy = psys[idx]
        pz = pzs[idx]

        start_point = (sy1, 0)
        end_point = (sy2, 0)
        middle_point = (psy, pz)

        control_points = (start_point, middle_point, end_point)
        sur_y, sur_z = _get_curve_line(control_points)

        sur_x = [sx1] * len(sur_y)

        model_data.append((sur_x, sur_y, sur_z))

    if enable_debug is True:
        xmax = max(sxs)
        ymax = max(sys)
        lim_value = max([xmax, ymax])
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        ax.set_title("rain drop model Model")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')
        ax.set_zlim(0, lim_value)
        ax.set_xlim(0, lim_value)
        ax.set_ylim(0, lim_value)
        for idx, data in enumerate(model_data):
            if idx % 20 == 0:
                ax.plot(data[0], data[1], data[2], 'b-')

        plt.show()

    return model_data


def convert_model_to_mesh(model_data, enable_debug=False):
    """
    generate the mesh based on the model data
    :param model_data: the model data used to generate the data
    :param enable_debug: whether this function enable debugging
    :return:
    """
    model_data_length = len(model_data)
    mesh_width = len(model_data[0][0])

    sample_model_step = np.ceil(model_data_length / mesh_width)
    mesh_width = int(model_data_length / sample_model_step)

    mesh_data = np.zeros([mesh_width, mesh_width, 3])

    max_x = 0
    max_y = 0
    max_z = 0

    for h_idx in range(model_data_length):
        if h_idx % sample_model_step == 0:
            sampled_h_idx = int(h_idx / sample_model_step)
            for w_idx in range(mesh_width):
                mesh_point_x = model_data[h_idx][0][w_idx]
                mesh_point_y = model_data[h_idx][1][w_idx]
                mesh_point_z = model_data[h_idx][2][w_idx]

                mesh_data[sampled_h_idx][w_idx][0] = mesh_point_x
                mesh_data[sampled_h_idx][w_idx][1] = mesh_point_y
                mesh_data[sampled_h_idx][w_idx][2] = mesh_point_z

                if max_x < mesh_point_x:
                    max_x = mesh_point_x

                if max_y < mesh_point_y:
                    max_y = mesh_point_y

                if max_z < mesh_point_z:
                    max_z = mesh_point_z

    if enable_debug is True:
        lim_value = max([max_y, max_x, max_z])
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        ax.set_title("rain drop Mesh - 3D")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')
        ax.set_xlim(0, lim_value)
        ax.set_ylim(0, lim_value)
        ax.set_zlim(0, lim_value)
        ax.plot_wireframe(mesh_data[:, :, 0], mesh_data[:, :, 1], mesh_data[:, :, 2], color='b')

        plt.show()

    return mesh_data


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

    current_pt = mesh_data[current_h_idx][current_w_idx]
    left_pt = mesh_data[current_h_idx][current_w_idx - 1]
    right_pt = mesh_data[current_h_idx][current_w_idx + 1]
    bottom_pt = mesh_data[current_h_idx + 1][current_w_idx]
    up_pt = mesh_data[current_h_idx - 1][current_w_idx]

    left_vector = current_pt - left_pt
    right_vector = right_pt - current_pt
    up_vector = current_pt - up_pt
    bottom_vector = bottom_pt - current_pt

    left_up_normal = np.cross(left_vector, up_vector)
    left_bottom_normal = np.cross(left_vector, bottom_vector)
    right_up_normal = np.cross(right_vector, up_vector)
    right_bottom_normal = np.cross(right_vector, bottom_vector)

    left_up_normal = _normalize_vector(left_up_normal)
    left_bottom_normal = _normalize_vector(left_bottom_normal)
    right_up_normal = _normalize_vector(right_up_normal)
    right_bottom_normal = _normalize_vector(right_bottom_normal)

    final_normal = (left_up_normal + left_bottom_normal + right_up_normal + right_bottom_normal) / 4

    final_normal = _normalize_vector(final_normal)

    return final_normal


def compute_normal_based_on_mesh(mesh_data, enable_debug=False, batch_to_debug_show=20, debug_show_scale=4):
    height = mesh_data.shape[0]
    width = mesh_data.shape[1]

    normal_data = np.zeros([height, width, 3])

    max_x = 0
    max_y = 0
    max_z = 0

    for h_idx in range(height):
        for w_idx in range(width):
            if 0 < h_idx < height-1 and 0 < w_idx < width-1:
                norm = _compute_normal(mesh_data, h_idx, w_idx)
                normal_data[h_idx][w_idx] = norm

                if max_x < mesh_data[h_idx][w_idx][0]:
                    max_x = mesh_data[h_idx][w_idx][0]

                if max_y < mesh_data[h_idx][w_idx][1]:
                    max_y = mesh_data[h_idx][w_idx][1]

                if max_z < mesh_data[h_idx][w_idx][2]:
                    max_z = mesh_data[h_idx][w_idx][2]

    if enable_debug is True:
        batch_step = int(height / batch_to_debug_show)
        lim_value = max([max_x, max_y, max_z])
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        ax.set_title("rain drop Mesh - 3D")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')
        ax.set_xlim(0, lim_value)
        ax.set_ylim(0, lim_value)
        ax.set_zlim(0, lim_value)
        ax.plot_wireframe(mesh_data[:, :, 0], mesh_data[:, :, 1], mesh_data[:, :, 2], color='b')
        for m_h_idx in range(mesh_data.shape[0]):
            for m_w_idx in range(mesh_data.shape[1]):
                if m_h_idx % batch_step == 0 and m_w_idx % batch_step == 0:
                    normal = normal_data[m_h_idx][m_w_idx] * debug_show_scale
                    point = mesh_data[m_h_idx, m_w_idx]
                    ax.quiver(point[0], point[1], point[2], normal[0], normal[1], normal[2], color='r')

        plt.show()

    return normal_data


def generate_drop_model(data_file, length, width, height, peak_offset, lines_step=0.001):
    shape_x_list, shape_y_list, peak_shape_y_list, peak_z_list = \
        read_drop_data(data_file, length, width, height, peak_offset, lines_step, enable_debug=False)
    rain_drop_model = generate_3d_model(shape_x_list, shape_y_list, peak_shape_y_list, peak_z_list, enable_debug=False)
    rain_drop_mesh = convert_model_to_mesh(rain_drop_model, enable_debug=False)
    rain_drop_normal = compute_normal_based_on_mesh(rain_drop_mesh, enable_debug=False)

    return rain_drop_mesh, rain_drop_normal


# A sample code to test through tehe script
"""
shape_x_list, shape_y_list, peak_shape_y_list, peak_z_list = read_drop_data(reader.DEBUG_TEST_MAP, enable_debug=False)
rain_drop_model = generate_3d_model(shape_x_list, shape_y_list, peak_shape_y_list, peak_z_list, enable_debug=False)
rain_drop_mesh = convert_model_to_mesh(rain_drop_model, enable_debug=False)
rain_drop_normal = compute_normal_based_on_mesh(rain_drop_mesh, enable_debug=True)
"""
