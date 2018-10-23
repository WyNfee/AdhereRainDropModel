"""
Author: Yifei Wang
Date: 2018-10-23
Description: This script will extract the data in met file for adhere rain drop model construction
"""

import cv2
import numpy as np
from collections import namedtuple
import bezier_curve
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline

Point = namedtuple("Point", ["X", "Y", "Color"])
ControlPoints = namedtuple("ControlPoints", ["HeightCurve", "HeightPeak","ShapeCurveA", "ShapeCurveB"])

COLOR_TYPE_DICT = {
    "RED": 1,
    "BLUE": 2,
    "BLACK": 3,
    "WHITE": 4,
    "GREEN": 5
}


def read_met_file(file_path):
    file_data = cv2.imread(file_path)
    return file_data


def _get_data_point_color(data_point):
    r_channel = data_point[2] / 255
    g_channel = data_point[1] / 255
    b_channel = data_point[0] / 255

    r_ratio = r_channel / (r_channel + g_channel + b_channel + 1e-6)
    b_ratio = b_channel / (r_channel + g_channel + b_channel + 1e-6)
    g_ratio = g_channel / (r_channel + g_channel + b_channel + 1e-6)

    color_type = COLOR_TYPE_DICT["WHITE"]

    if r_ratio > 0.6:
        color_type = COLOR_TYPE_DICT["RED"]
    elif b_ratio > 0.6:
        color_type = COLOR_TYPE_DICT["BLUE"]
    elif g_ratio > 0.6:
        color_type = COLOR_TYPE_DICT["GREEN"]
    elif r_ratio < 0.1 and b_ratio < 0.1 and g_ratio < 0.1:
        color_type = COLOR_TYPE_DICT["BLACK"]

    return color_type


def extract_data(file_data):
    file_data_mask = np.ones([file_data.shape[0], file_data.shape[1]], dtype=np.int32)

    height = file_data.shape[0]
    width = file_data.shape[1]

    valid_data_list = list()

    h_idx = 0
    w_idx = 0
    while h_idx < height:
        while w_idx < width:
            data_point = file_data[h_idx][w_idx]
            color_type = _get_data_point_color(data_point)
            if color_type != COLOR_TYPE_DICT["WHITE"] and file_data_mask[h_idx][w_idx] == 1:
                valid_data_list.append(Point(w_idx, h_idx, color_type))

                for mk_down_idx_h in range(6):
                    for mk_down_idx_w in range(6):
                        file_data_mask[h_idx + mk_down_idx_h - 2][w_idx + mk_down_idx_w - 2] = 0

            w_idx = w_idx + 1

        h_idx = h_idx + 1
        w_idx = 0

    return valid_data_list


def _plot_data_point(data_points, height, width):

    plot_plate = np.ones([height, width, 3], dtype=np.uint8) * 255

    for point in data_points:
        x_idx = point.X
        y_idx = point.Y
        c_idx = point.Color

        if c_idx == 1:
            color = (0, 0, 255)      # Red
        elif c_idx == 2:
            color = (255, 0, 0)      # Blue
        elif c_idx == 3:
            color = (0, 0, 0)        # Black
        elif c_idx == 4:
            color = (255, 255, 255)  # White
        elif c_idx == 5:
            color = (0, 255, 0)      # Green

        plot_plate[y_idx][x_idx] = color
        cv2.circle(plot_plate, (x_idx, y_idx), 4, color)

    cv2.imshow("DATA POINT RECONSTRUCT", plot_plate)
    cv2.waitKey(-1)


def _sort_points_by_x_axis(e):
    return e.X


def _generate_curve_common_control_points(colored_points):
    points_list = colored_points

    points_list.sort(key=_sort_points_by_x_axis)

    x_list = [p.X for p in points_list]
    y_list = [p.Y for p in points_list]

    max_x = max(x_list)
    min_x = min(x_list)
    width = max_x - min_x

    max_y = max(y_list)
    min_y = min(y_list)
    height = max_y - min_y

    x_list = [(x - min_x) / width for x in x_list]
    y_list = [(max_y - y) / height for y in y_list]

    control_points = list()

    for idx in range(len(x_list)):
        control_points.append((x_list[idx], y_list[idx]))

    return control_points


def _plot_common_cubic_curve(control_points):
    x_coord = [p[0] for p in control_points]
    y_coord = [p[1] for p in control_points]
    cs = CubicSpline(x_coord, y_coord)
    xs = np.arange(0, 1, 0.001)
    plt.plot(x_coord, y_coord, 'o', label='data')
    plt.plot(xs, cs(xs), label="S")
    plt.show()


def _generate_curve_shape_control_points(colored_points):
    points_list = colored_points
    points_list.sort(key=_sort_points_by_x_axis)

    x_list = [p.X for p in points_list]
    y_list = [p.Y for p in points_list]

    max_x = max(x_list)
    min_x = min(x_list)
    width = max_x - min_x

    max_y = max(y_list)
    min_y = min(y_list)
    height = max_y - min_y

    x_list = [((x - min_x)/width - 0.5) * 2 for x in x_list]  # convert x_list to [-1, 1]
    y_list = [((y - min_y)/height - 0.5) * 2 for y in y_list] # convert y to [-1,1]

    # a bunch of hacking to access the data in order to generate a clockwise data point access order
    list_length = len(x_list)
    access_index_range = range(list_length)
    access_list = list()
    acc_idx = 0

    while acc_idx < list_length:
        data = 2 * acc_idx + 1
        if data < list_length:
            access_list.append(access_index_range[data])
        else:
            access_list.append(access_index_range[list_length - 1 - data])
        acc_idx += 1

    access_list = [0] + access_list
    #access_list = [0, 1, 3, 5, 7, 9, 8, 6, 4, 2, 0]

    check_point_list = list()

    for acc_idx in access_list:
        check_point_list.append((x_list[acc_idx], y_list[acc_idx]))

    return check_point_list


def _plot_shape_cubic_curve(shape_curve):

    x_collection = [data[0] for data in shape_curve]
    y_collection = [data[1] for data in shape_curve]

    theta = 2 * np.pi * np.linspace(0, 1, 11)
    cs = CubicSpline(theta, shape_curve, bc_type='periodic')
    xs = 2 * np.pi * np.linspace(0, 1, 200)
    plt.plot(x_collection, y_collection, 'o', label='data')
    plt.plot(cs(xs)[:, 0], cs(xs)[:, 1], label='spline')
    plt.show()


def convert_data_points_to_control_points(data_points):
    green_points = list()
    black_points = list()
    red_points = list()
    blue_points = list()

    for point in data_points:
        pc = point.Color
        if pc == 1:
            red_points.append(point)
        elif pc == 2:
            blue_points.append(point)
        elif pc == 3:
            black_points.append(point)
        elif pc == 5:
            green_points.append(point)
        else:
            raise ValueError("Not support color exists in color points")

    height_curve = _generate_curve_common_control_points(red_points)
    height_peak = _generate_curve_common_control_points(blue_points)
    shape_curve = _generate_curve_shape_control_points(black_points)

    _plot_common_cubic_curve(height_curve)
    _plot_common_cubic_curve(height_peak)
    _plot_shape_cubic_curve(shape_curve)


met_data = read_met_file(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\test.bmp")
valid_data_point = extract_data(met_data)
_plot_data_point(valid_data_point, met_data.shape[0], met_data.shape[1])
convert_data_points_to_control_points(valid_data_point)

cv2.imshow("data", met_data)
cv2.waitKey(-1)