"""
Author: Yifei Wang
Date: 2018-10-24
Description: This script will generate a water drop model
"""

import met_file_reader as reader
import bezier_curve as bezier

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D


def _get_curve_line(control_points):
    axis_1_points, axis_2_points = bezier.generate_bezier_curve(control_points)
    return axis_1_points, axis_2_points


def read_drop_data(data_file, length=100, width=100, height=20, peak_shape_offset=0.5, line_step=0.001, enable_debug=False):
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
    sxs: outline of rain drop (x, y coordinates) x coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    sys: outline of rain drop (x, y coordinates), y coordinates, should be 4 times of based sampling, that is 4 * 1 / line_step;
    psys: the peak shape projected on y axis, should be 2 times of base sampling that is 2 * 1 / line_step;
    pzs: the peak height of peak, on z axis, should be 2 times of base sampling, that is 2 * 1 / line_step
    :param enable_debug: whether enable debug plot
    :return: a list of lists, length is the sampling over x axis.
    each element of list, contains 3 other lists, each represent the list of points on x axis, y axis, and z axis,
    each list contains 1000 elements by default
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
        fig = plt.figure()
        ax = fig.gca(projection='3d')
        ax.set_title("rain drop model Model")
        ax.set_xlabel('X axis')
        ax.set_ylabel('Y axis')
        ax.set_zlabel('Z axis')
        ax.set_zlim(0, 100)
        for idx, data in enumerate(model_data):
            if idx % 20 == 0:
                ax.plot(data[0], data[1], data[2], 'b-')

        plt.show()

    return model_data


sxs, sys, psys, pzs = read_drop_data(reader.DEBUG_TEST_MAP, enable_debug=False)
rain_drop_model = generate_3d_model(sxs, sys, psys, pzs, enable_debug=True)
