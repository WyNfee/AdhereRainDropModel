"""
Author: Yifei Wang
Date: 2018-10-23
Description: This script will extract the data in met file for adhere rain drop model construction
"""

import cv2
import numpy as np
from collections import namedtuple

Point = namedtuple("Point", ["X", "Y", "Color"])

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


met_data = read_met_file(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\rain_drop_data.bmp")
valid_data_point = extract_data(met_data)
_plot_data_point(valid_data_point, met_data.shape[0], met_data.shape[1])


cv2.imshow("data", met_data)
cv2.waitKey(-1)