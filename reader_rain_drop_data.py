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

# shape out map directory
OUT_SHAPE_DIRECTORY = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\outshape"

# save read data location
DIR_SAVE_MAP_DATA = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved"

# saved file name of peak height
SAVE_FILE_NAME_PEAK_HEIGHT = "peak_height"

# save file name of peak shape
SAVE_FILE_NAME_PEAK_SHAPE = "peak_shape"

# save file name of out shape
SAVE_FILE_NAME_OUT_SHAPE = "out_shape"

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


def _extract_single_data(single_image_path, sample_area=5):
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

                for mk_down_idx_h in range(sample_area):
                    for mk_down_idx_w in range(sample_area):
                        file_data_mask[h_idx + mk_down_idx_h - int(sample_area/2)][w_idx + mk_down_idx_w - int(sample_area/2)] = 0

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
    return e.X + 1 / e.Y


def _arrange_data_points(data_points_list, form='line'):
    """
    merge the duplicated sampled points, so that make it possible for function simulation
    :param data_points_list: the data points to do this arrangement
    :param form: the form for this arrange process
    :return: a list of arranged data points
    """
    arranged_data_points_list = list()

    data_points_list.sort(key=__sort_data_points_key)
    current_x = 0
    current_y_result_list = list()

    if form == 'line':
        # the line form, is only allow one value share the same x-axis value,
        # if not, mean the all possible y value
        for data in data_points_list:
            current_y_result_list.append(data.Y)
            if data.X == current_x:
                continue
            else:
                sum_y = sum(current_y_result_list)
                y_count = len(current_y_result_list)
                avg_y = sum_y / y_count
                point = Point(X=data.X, Y=avg_y)
                arranged_data_points_list.append(point)

                current_y_result_list = list()
                current_x = data.X
    elif form == 'cycle':
        # the cycle form, only allow 2 value share the same x-axis value,
        # if not,
        # 1 value will be removed
        # 2 or more values, will remove the additional value, only take the boundary values
        current_x_list = list()
        current_x = data_points_list[0].X
        for data in data_points_list:
            if data.X != current_x:
                current_x_amount = len(current_x_list)

                # this is a pair operation, only 2 points share same x axis will be recorded
                if current_x_amount >= 2:
                    min_y = min(current_y_result_list)
                    max_y = max(current_y_result_list)

                    point = Point(X=current_x, Y=min_y)
                    arranged_data_points_list.append(point)

                    point = Point(X=current_x, Y=max_y)
                    arranged_data_points_list.append(point)

                current_y_result_list = list()
                current_x_list = list()
                current_x = data.X

            current_x_list.append(data.X)
            current_y_result_list.append(data.Y)
    else:
        raise ValueError("Will not support other arrange forms")

    return arranged_data_points_list


def _split_cycle_points_to_splines(arranged_cycle_points):
    """
    will split the cycle points into two sets of splines, the upper and bottom one
    NOTE: will not perform cycle points check, behavior is undefined if do this for non-arranged cycle points set
    :param arranged_cycle_points: the arranged cycle points set
    :return: two list, one is upper list, the other is bottom list
    """
    upper_list = list()
    bottom_list = list()

    # for the cycle line, we have to set the first and last to be a single point
    # to make it a cycle
    first_point_x = arranged_cycle_points[0].X - 1
    first_point_y_1 = arranged_cycle_points[0].Y
    first_point_y_2 = arranged_cycle_points[1].Y
    last_point_x = arranged_cycle_points[-1].X + 1
    last_point_y_1 = arranged_cycle_points[-1].Y
    last_point_y_2 = arranged_cycle_points[-2].Y
    first_point = Point(X=first_point_x, Y=(first_point_y_1 + first_point_y_2) / 2)
    last_point = Point(X=last_point_x, Y=(last_point_y_1 + last_point_y_2) / 2)

    upper_list.append(first_point)
    bottom_list.append(first_point)

    idx = 0
    points_amount = len(arranged_cycle_points)

    while idx < points_amount:
        upper_point = arranged_cycle_points[idx]
        idx += 1
        bottom_point = arranged_cycle_points[idx]
        idx += 1

        upper_list.append(upper_point)
        bottom_list.append(bottom_point)

    upper_list.append(last_point)
    bottom_list.append(last_point)

    return upper_list, bottom_list


def _get_max_scale_reference(upper_list, bottom_list):
    """
    find the scale reference for both upper and bottom list,
    otherwise, in equal reference value will cause normalize for upper and bottom spline
    disagree on final output
    :param upper_list: the upper list to operate
    :param bottom_list: the bottom list to operate
    :return: a scalar, the reference to work on normalize the two points set
    """
    base_value = upper_list[0][1]
    reference_upper_list = [abs(u[1] - base_value) for u in upper_list]
    reference_bottom_list = [abs(u[1] - base_value) for u in bottom_list]

    upper_reference = max(reference_upper_list)
    bottom_reference = max(reference_bottom_list)

    reference_value = max([upper_reference, bottom_reference])

    return reference_value


def _format_data_points(data_points_list, form='height', reference_scale=None):
    """
    format the real number data point,
    form is 'height', normalized to [0, 1]
    form is 'shape', normalize to [-1, 1]
    :param data_points_list: the data points to do normalization
    :param form: the form of this format operation. default to height
    :param reference_scale: only works with form is 'out',
    to make splines process on same normalize scale,
    if not provided, form 'out' will be separately deal with each spline with its own normalize scale
    :return: a list of normalized data points
    """
    data_points_list.sort(key=__sort_data_points_key)

    x_list = [p.X for p in data_points_list]
    y_list = [p.Y for p in data_points_list]

    max_x = max(x_list)
    min_x = min(x_list)
    width = max_x - min_x

    x_list = [((x - min_x) / width) for x in x_list]

    # note: y index in image direction, min_y is "upper" to max_y
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
    elif form == 'out':
        base_height = y_list[0]
        y_list = [(base_height - y) for y in y_list]

        if reference_scale is None:
            temp_y_list = [abs(y) for y in y_list]
            max_y = max(temp_y_list)
        else:
            max_y = reference_scale

        y_list = [(y / max_y) for y in y_list]
    else:
        raise ValueError("Will not support such format methods")

    control_points = list()

    for idx in range(len(x_list)):
        control_points.append((x_list[idx], y_list[idx]))

    return control_points


def _combine_upper_and_bottom_together(upper_control_points, bottom_control_points):
    """
    combine two sets of control points into one, to support cycle operation
    the reading of these points are clock-wise, from upper to bottom, assuming the reading process, like:
    9, 10, 12, 1, 2,... ,7, 8, 9 on the clock
    NOTE: will not do any check for upper and bottom control points,
    the behavior will be undefined if data provided is incorrect
    :param upper_control_points: the upper control points
    :param bottom_control_points: the bottom control points
    :return: a formatted spline line control points
    """
    _combined_control_points = list()
    # clock wise reading not need to change on upper ones
    _combined_control_points.extend(upper_control_points)
    # need to reverse processing the bottom ones,
    # but the last one, which is duplicated
    # first one is not duplicated, for cycle process and middleware requirments
    bottom_control_points.pop(-1)

    for point in reversed(bottom_control_points):
        _combined_control_points.append(point)

    return _combined_control_points


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


def _extract_category_and_peak_shape_count_from_name(file_name):
    """
    extract the category and count of a peak shape
    :param file_name: the file name to work with, must be a format with "[category]_[count].xxx"
    :return: category name, count name
    """
    file_safe_name = file_name.split('.')[0]
    name_elements = file_safe_name.split('_')
    category = int(name_elements[0])
    count = int(name_elements[1])

    return category, count


def _extract_category_from_name(file_name):
    """
    extract the category of a out shape
    :param file_name: the file name to work with, must be a format "[category].xxx"
    :return: category name
    """
    return int(file_name.split('.')[0])


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


def _plot_shape_cubic_curve(shape_curve):
    """
    plot the cubic periodic curve to preview
    :param shape_curve:the control points to work with
    :return: None
    """
    x_collection = [data[0] for data in shape_curve]
    y_collection = [data[1] for data in shape_curve]

    control_points_lenth = len(shape_curve)

    theta = 2 * np.pi * np.linspace(0, 1, control_points_lenth)
    cs = CubicSpline(theta, shape_curve, bc_type='periodic')
    xs = 2 * np.pi * np.arange(0, 1, 0.001)
    plt.plot(x_collection, y_collection, 'o', label='data')
    plt.plot(cs(xs)[:, 0], cs(xs)[:, 1], label='spline')
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
        arranged_point_list = _arrange_data_points(data_point_list, form='line')
        formatted_data_point_list = _format_data_points(arranged_point_list, form='height')
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
        shape_category, shape_count = _extract_category_and_peak_shape_count_from_name(data.NAME)
        data_point_list = _extract_single_data(data.PATH)
        arranged_point_list = _arrange_data_points(data_point_list, form='line')
        formatted_data_point_list = _format_data_points(arranged_point_list, 'shape')
        sampled_control_points = _sample_curve_to_generate_control_points(formatted_data_point_list, sample_list)
        peak_data[idx, :sample_amount, :] = sampled_control_points
        peak_data[idx, sample_amount, 0] = shape_category
        peak_data[idx, sample_amount, 1] = shape_count
        if enable_debugging:
            _plot_common_cubic_curve(sampled_control_points)

    np.save(save_file_path, peak_data)


def generate_normalized_out_shape(data_list, sample_list, save_dir, save_file_name, enable_debugging=False):
    data_amount = len(data_list)
    sample_amount = len(SAMPLE_X_LIST)
    save_file_path = os.path.join(save_dir, save_file_name)

    # note, we sample sample_amount * 2 is actually numerical equal
    # sample_amount + sample_amount - 1 for actual control points store slots
    # 1 for category name slot
    shape_data = np.zeros([data_amount, sample_amount * 2, 2])

    for idx, data in enumerate(data_list):
        category_name = _extract_category_from_name(data.NAME)
        data_point_list = _extract_single_data(data.PATH, sample_area=1)
        arranged_point_list = _arrange_data_points(data_point_list, form='cycle')
        upper_list, bottom_list = _split_cycle_points_to_splines(arranged_point_list)
        scale_reference = _get_max_scale_reference(upper_list, bottom_list)
        format_upper_list = _format_data_points(upper_list, 'out', scale_reference)
        format_bottom_list = _format_data_points(bottom_list, 'out', scale_reference)
        sampled_upper_control_points = _sample_curve_to_generate_control_points(format_upper_list, sample_list)
        sampled_bottom_control_points = _sample_curve_to_generate_control_points(format_bottom_list, sample_list)
        sampled_whole_cycle_points = _combine_upper_and_bottom_together(sampled_upper_control_points, sampled_bottom_control_points)

        shape_data[idx, :sample_amount * 2 - 1, :] = sampled_whole_cycle_points
        shape_data[idx, sample_amount * 2 - 1, 0] = category_name

        if enable_debugging:
            _plot_shape_cubic_curve(sampled_whole_cycle_points)

    np.save(save_file_path, shape_data)


peak_height_data_list = collect_all_image_file(PEAK_HEIGHT_DIRECTORY)
generate_normalized_peak_height(peak_height_data_list, SAMPLE_X_LIST, DIR_SAVE_MAP_DATA, SAVE_FILE_NAME_PEAK_HEIGHT, False)

peak_shape_data_list = collect_all_image_file(PEAK_SHAPE_DIRECTORY)
generate_normalized_peak_shape(peak_shape_data_list, SAMPLE_X_LIST, DIR_SAVE_MAP_DATA, SAVE_FILE_NAME_PEAK_SHAPE, False)

out_shape_data_list = collect_all_image_file(OUT_SHAPE_DIRECTORY)
generate_normalized_out_shape(out_shape_data_list, SAMPLE_X_LIST, DIR_SAVE_MAP_DATA, SAVE_FILE_NAME_OUT_SHAPE, False)

print("complete!")
