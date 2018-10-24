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
    h_curve_points, h_peak_points, shape_curve_points = reader.generate_curve_list(data_file, line_step)

    sxs = [p[0] * width for p in shape_curve_points]
    sys = [p[1] * length for p in shape_curve_points]
    psys = [p[1] * length * peak_shape_offset for p in h_curve_points]
    pzs = [p[1] * height for p in h_peak_points]

    if enable_debug is True:
        plt.plot(sxs, sys, label='shape')
        plt.plot(sxs[0:2000], psys, label='peak_shape')
        plt.plot(sxs[0:2000], pzs, label='peak')
        plt.show()

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

    fig = plt.figure()
    ax = fig.gca(projection='3d')
    ax.set_title("rain drop model Model")
    ax.set_xlabel('X axis')
    ax.set_ylabel('Y axis')
    ax.set_zlabel('Z axis')
    ax.set_zlim(0, 100)
    for idx, data in enumerate(model_data):
        if idx % 50 == 0:
            ax.plot(data[0], data[1], data[2], 'b-')

    plt.show()


    #plt.plot(sxs[0:2000], sys[0:2000], label='half_i')
    #plt.plot(sxs[2000:], sys[2000:], label='half_ii')
    #plt.plot([sxs[0], sxs[2000]], [sys[0], sys[2000]], label='line')
    #plt.plot([sxs[100], sxs[-100]], [sys[100], sys[-100]], label='line2')
    #plt.show()


read_drop_data(reader.DEBUG_TEST_MAP, enable_debug=False)

