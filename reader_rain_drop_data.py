"""
Author: Yifei Wang
Date: 2018-10-29
Description: this script will read all rain drop model data
"""

import os
import cv2
import numpy as np
from collections import namedtuple
import matplotlib.pyplot as plt
from scipy.interpolate import CubicSpline

Point = namedtuple("Point", ["X", "Y"])
FileProperty = namedtuple("FileProperty", ["PATH", "NAME"])

# height peak map directory
PEAK_HEIGHT_DIRECTORY = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\height"

# shape peak map directory
PEAK_SHAPE_DIRECTORY = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\peakshape"

# save read data location
DIR_SAVE_MAP_DATA = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved"

# saved file name of peak height
SAVE_FILE_NAME_PEAK_HEIGHT = "peak_height"

# save file name of peak shape
SAVE_FILE_NAME_PEAK_SHAPE = "peak_shape"

# since range is [min, max), so we have to set as 1.1
# (actually, (1.0, 1.1] is always OK to generate correct index)
# to change the range, the more list we have, the more accurate spline will reproduce/sampled
SAMPLE_X_LIST = np.arange(0, 1.1, 0.1)


def _is_background_color(data_point):
    """
    Check whether a color is background or not, if the color is whitish,
    then it is the background
    :param data_point: the data point (rgb) to test
    :return: whether is is background or not (True/False)
    """
    r_channel = data_point[2] / 255
    g_channel = data_point[1] / 255
    b_channel = data_point[0] / 255

    if r_channel > 0.8 and g_channel > 0.8 and b_channel > 0.8:
        return True
    else:
        return False


def _extract_single_data(single_image_path):
    """
    process a input image, extract the sample point from the image
    :param single_image_path: a image path to operate with
    :return: a key point list
    """
    image_data = cv2.imread(single_image_path)
    image_height = image_data.shape[0]
    image_width = image_data.shape[1]
    file_data_mask = np.ones([image_height, image_width], dtype=np.int32)

    valid_data_list = list()

    h_idx = 0
    w_idx = 0
    while w_idx < image_width:
        while h_idx < image_height:

            data_point = image_data[h_idx][w_idx]
            is_background = _is_background_color(data_point)

            if is_background is False and file_data_mask[h_idx][w_idx] == 1:
                valid_data_list.append(Point(X=w_idx, Y=h_idx))

                for mk_down_idx_h in range(5):
                    for mk_down_idx_w in range(5):
                        file_data_mask[h_idx + mk_down_idx_h - 2][w_idx + mk_down_idx_w - 2] = 0

            h_idx = h_idx + 1
        w_idx = w_idx + 1
        h_idx = 0

    return valid_data_list


def __sort_data_points_key(e):
    """
    sort key function used for list.sort
    :param e: the element for compare of sort function, only work with Point namedtuple
    :return: the result of each compare value
    """
    return e.X


def _format_data_points(data_points_list, form='height'):
    """
    format the real number data point,
    form is 'height', normalized to [0, 1]
    form is 'shape', normalize to [-1, 1]
    :param data_points_list: the data points to do normalization
    :return: a list of normalized data points
    """
    data_points_list.sort(key=__sort_data_points_key)

    x_list = [p.X for p in data_points_list]
    y_list = [p.Y for p in data_points_list]

    max_x = max(x_list)
    min_x = min(x_list)
    width = max_x - min_x

    x_list = [((x - min_x) / width) for x in x_list]

    # note: y index in image direction, min_y is "upper" of max_y
    if form == 'height':
        max_y = max(y_list)
        min_y = min(y_list)
        height = max_y - min_y

        y_list = [((max_y - y) / height) for y in y_list]
        y_list[0] = y_list[-1] = 0
    elif form == 'shape':
        base_height = y_list[0]
        y_list = [(base_height - y) for y in y_list]
        temp_y_list = [abs(y) for y in y_list]
        max_y = max(temp_y_list)

        y_list = [(y / max_y) for y in y_list]
        y_list[0] = y_list[-1] = 0
    else:
        raise ValueError("Will not support such format methods")

    control_points = list()

    for idx in range(len(x_list)):
        control_points.append((x_list[idx], y_list[idx]))

    return control_points


def _sample_curve_to_generate_control_points(control_points, sample_list):
    """
    use controls points to generate a cubic spline, and sample the spline to get new control points
    :param control_points: teh control points to generate teh spline
    :param sample_list: the sample list of numbers along x axis
    :return: sampled control points
    """
    x_coord = [p[0] for p in control_points]
    y_coord = [p[1] for p in control_points]
    cs = CubicSpline(x_coord, y_coord)

    sampled_control_points = list()

    for idx in range(len(SAMPLE_X_LIST)):
        sampled_control_points.append([sample_list[idx], cs(sample_list[idx])])

    sampled_control_points[-1][-1] = control_points[-1][-1]
    return sampled_control_points


def _extract_category_and_peak_shape_count(file_name):
    """
    extract the category and count of a peak shape
    :param file_name: the file name to work with, must be a format with "[category]_[count].xxx"
    :return:
    """
    file_safe_name = file_name.split('.')[0]
    name_elements = file_safe_name.split('_')
    category = int(name_elements[0])
    count = int(name_elements[1])

    return category, count


def _plot_common_cubic_curve(control_points):
    """
    plot the cubic curve to preview
    :param control_points: the control points to work with
    :return: None
    """
    x_coord = [p[0] for p in control_points]
    y_coord = [p[1] for p in control_points]
    cs = CubicSpline(x_coord, y_coord)
    xs = np.arange(0, 1, 0.001)
    plt.plot(x_coord, y_coord, 'o', label='data')
    plt.plot(xs, cs(xs), label="S")
    plt.show()


def collect_all_image_file(dir_image_file):
    """
    search the root of given directory, and find all "bmp" file
    return a list of these file paths
    :param dir_image_file: the directory of image file
    :return: a list of paths of valid bmp file, as FileProperty
    """
    image_path_list = list()
    files_in_dir = os.listdir(dir_image_file)
    for file in files_in_dir:
        if file.endswith(".bmp") is True:
            file_path = os.path.join(dir_image_file, file)
            file_property = FileProperty(PATH=file_path, NAME=file)
            image_path_list.append(file_property)

    return image_path_list


def generate_normalized_peak_height(data_list, sample_list, save_dir, save_file_name, enable_debugging=False):
    """
    generate peak shape data with [0,1] normalized along x, y axis
    :param data_list: the data points list extract from input image
    :param sample_list: the sample list along the axis
    :param save_dir: the directory to save the peak data
    :param save_file_name: the file name of peak data to save
    :param enable_debugging: whether enable debugging plot for this process
    :return: None
    """
    data_amount = len(data_list)
    sample_amount = len(SAMPLE_X_LIST)

    peak_data = np.zeros([data_amount, sample_amount, 2])

    save_file_path = os.path.join(save_dir, save_file_name)

    for idx, data in enumerate(data_list):
        data_point_list = _extract_single_data(data.PATH)
        formatted_data_point_list = _format_data_points(data_point_list)
        sampled_control_points = _sample_curve_to_generate_control_points(formatted_data_point_list, sample_list)
        peak_data[idx, :, :] = sampled_control_points
        if enable_debugging:
            _plot_common_cubic_curve(sampled_control_points)

    np.save(save_file_path, peak_data)


def generate_normalized_peak_shape(data_list, sample_list, save_dir, save_file_name, enable_debugging=False):
    """
    generate peak shape data with [0,1] normalized along x axis, and [-1, 1] along y axis

    :param data_list: the data points list extract from input image
    :param sample_list: the sample list along the axis
    :param save_dir: the directory to save the peak data
    :param save_file_name: the file name of peak data to save
    :param enable_debugging: whether enable debugging plot for this process
    :return: None
    """
    data_amount = len(data_list)
    sample_amount = len(SAMPLE_X_LIST)
    save_file_path = os.path.join(save_dir, save_file_name)

    # for peak shape, have to store two more values, the shape name, and its index
    peak_data = np.zeros([data_amount, sample_amount+1, 2])

    for idx, data in enumerate(data_list):
        shape_category, shape_count = _extract_category_and_peak_shape_count(data.NAME)
        data_point_list = _extract_single_data(data.PATH)
        formatted_data_point_list = _format_data_points(data_point_list, 'shape')
        sampled_control_points = _sample_curve_to_generate_control_points(formatted_data_point_list, sample_list)
        peak_data[idx, :sample_amount, :] = sampled_control_points
        peak_data[idx, sample_amount, 0] = shape_category
        peak_data[idx, sample_amount, 1] = shape_count
        if enable_debugging:
            _plot_common_cubic_curve(sampled_control_points)

    np.save(save_file_path, peak_data)
    

peak_height_data_list = collect_all_image_file(PEAK_HEIGHT_DIRECTORY)
generate_normalized_peak_height(peak_height_data_list, SAMPLE_X_LIST, DIR_SAVE_MAP_DATA, SAVE_FILE_NAME_PEAK_HEIGHT, False)

peak_shape_data_list = collect_all_image_file(PEAK_SHAPE_DIRECTORY)
generate_normalized_peak_shape(peak_shape_data_list, SAMPLE_X_LIST, DIR_SAVE_MAP_DATA, SAVE_FILE_NAME_PEAK_SHAPE, False)


# TODO: add drop shape process code
