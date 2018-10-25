"""
Author: Yifei Wang
Date: 2018-10-24
Description: This script will render water drop on an image
"""

import cv2
import numpy as np
import water_drop_model as model
import copy

DEBUG_ROAD_IMAGE = "/home/maxieye/Documents/Git/AdhereRainDropModel/real.jpg"
DEBUG_DROP_DATA = "/home/maxieye/Documents/Git/AdhereRainDropModel/test.bmp"


def read_image_map(data_file, kernel=(125, 125), enable_debug=False):
    """
    read the data file, and give base image and reference image
    :param data_file: the data file path to operation
    :param kernel: the gaussian kernel size
    :param enable_debug: whether enable debugging
    :return:
    data_image: the image data, numpy data structure, with shape [hwc]
    reference_image, the reference image data, numpy data structure with shape [hwc]
    """
    image_data = cv2.imread(data_file)
    blurred = cv2.GaussianBlur(image_data, kernel, 0)
    reference_image = np.flipud(blurred)
    if enable_debug is True:
        cv2.imshow("Reference", reference_image)
        cv2.imshow("GSB", blurred)
        cv2.imshow("origin", image_data)
        cv2.waitKey(-1)
    return image_data, reference_image


def construct_drop_model(data_file, length, width, height, peak_offset):
    """
    generate model based on drop shape data and parameters
    :param data_file: the data file used to define shape of drop
    :param length: the length of drop in size
    :param width: the width of drop in size
    :param height: the height of drop in size
    :param peak_offset: the offset of peak in size
    :return:
    mesh: the mesh of drop project on x, y platform
    normal:the normal of drop project on x, y platform
    """
    mesh, normal = model.generate_drop_model(data_file, length, width, height, peak_offset)
    return mesh, normal


def project_normal_data_image_space(mesh_data, normal_data):
    height = mesh_data.shape[0]
    width = mesh_data.shape[1]
    channel = normal_data.shape[2]

    project_width = width + 4
    project_height = height + 4

    project_data = np.zeros([project_height, project_width, channel])
    project_data_mask = np.ones([project_height, project_width])

    for h_idx in range(height):
        for w_idx in range(width):
            point_data = mesh_data[h_idx][w_idx]
            pt_x = int(np.floor(point_data[0]))
            pt_y = int(np.floor(point_data[1]))

            project_y_idx = pt_y
            project_x_idx = pt_x

            if project_data_mask[project_y_idx][project_x_idx] == 1:  # note it is initialized with 1
                project_data[project_y_idx][project_x_idx] = normal_data[h_idx][w_idx]
                project_data_mask[project_y_idx][project_x_idx] = 0

    return project_data


def _mapping_normal_to_pixel(reference_image, reference_normal):
    image_height = reference_image.shape[0]
    image_width = reference_image.shape[1]

    norm_x = reference_normal[0]
    norm_y = reference_normal[1]
    norm_z = reference_normal[2]

    tan_x = norm_x / (norm_z + 1e-6)
    tan_y = norm_y / (norm_z + 1e-6)

    rad_x = np.arctan(tan_x)
    rad_y = np.arctan(tan_y)

    fraction_x = (rad_x / np.pi) + 0.5
    fraction_y = (rad_y / np.pi) + 0.5

    y_idx = int(image_height * fraction_y)
    x_idx = int(image_width * fraction_x)

    mapping_pixel = reference_image[y_idx][x_idx]

    return mapping_pixel


def create_rain_drop_on_image(input_image, reference_image, normal_data):

    water_drop_paste = copy.deepcopy(input_image)

    height = normal_data.shape[0]
    width = normal_data.shape[1]

    water_drop_paste_height = water_drop_paste.shape[0]
    water_drop_paste_width = water_drop_paste.shape[1]

    max_pixel_loc_x = 0
    min_pixel_loc_x = width

    max_pixel_loc_y = 0
    min_pixel_loc_y = height

    for h in range(height):
        for w in range(width):
            if normal_data[h][w][0] != 0 and normal_data[h][w][1] != 0 and normal_data[h][w][2] != 0:
                pixel_loc_x = int(water_drop_paste_width/2) + w
                pixel_loc_y = int(water_drop_paste_height/2) + h

                if pixel_loc_x < min_pixel_loc_x:
                    min_pixel_loc_x = pixel_loc_x

                if pixel_loc_x > max_pixel_loc_x:
                    max_pixel_loc_x = pixel_loc_x

                if pixel_loc_y < min_pixel_loc_y:
                    min_pixel_loc_y = pixel_loc_y

                if pixel_loc_y > max_pixel_loc_y:
                    max_pixel_loc_y = pixel_loc_y

                reference_value = reference_image[pixel_loc_y][pixel_loc_x]
                current_value = input_image[pixel_loc_y][pixel_loc_x]
                normal_z = normal_data[h][w][2]
                translate_factor = normal_z * 0.3

                drop_pixel = current_value * translate_factor + (1 - translate_factor) * reference_value

                water_drop_paste[pixel_loc_y][pixel_loc_x] = drop_pixel


    water_drop_paste = cv2.GaussianBlur(water_drop_paste, (31, 31), 0)
    #cv2.imshow('adding_water_drop', water_drop_paste)
    #cv2.waitKey(-1)

    # create a filtering matrix

    min_pixel_loc_y = min_pixel_loc_y - 10
    min_pixel_loc_x = min_pixel_loc_x - 10

    max_pixel_loc_y = max_pixel_loc_y + 10
    max_pixel_loc_x = max_pixel_loc_x + 10

    fill_area_height = max_pixel_loc_y - min_pixel_loc_y
    fill_area_width = max_pixel_loc_x - min_pixel_loc_x

    if fill_area_height % 2 == 0:
        fill_area_height = fill_area_height + 1

    if fill_area_width % 2 == 0:
        fill_area_width = fill_area_width + 1

    fill_area = np.zeros([fill_area_height, fill_area_width])

    half_max_width_index = int(max(range(fill_area_width)) / 2)
    half_max_height_index = int(max(range(fill_area_height)) / 2)

    for f_x_idx in range(fill_area_width):
        for f_y_idx in range(fill_area_height):
            x_factor = (half_max_width_index - abs(f_x_idx - half_max_width_index)) / half_max_width_index
            y_factor = (half_max_height_index - abs(f_y_idx - half_max_height_index)) / half_max_height_index
            fill_area[f_y_idx][f_x_idx] = x_factor / 2 + y_factor / 2

    # do filling
    for h in range(water_drop_paste_height):
        if h < min_pixel_loc_y or h >= max_pixel_loc_y:
            continue

        for w in range(water_drop_paste_width):
            if w < min_pixel_loc_x or w >= max_pixel_loc_x:
                continue

            current_value = input_image[h][w]
            reference_value = water_drop_paste[h][w]
            mask_filter_value = fill_area[h - min_pixel_loc_y][w - min_pixel_loc_x]

            fill_value = reference_value * mask_filter_value + (1 - mask_filter_value) * current_value
            #fill_value = (0, 0, 255 * mask_filter_value)

            input_image[h][w] = fill_value

    #cv2.imshow('ref', water_drop_paste)
    #cv2.imshow('final', input_image)
    #cv2.waitKey(-1)
    return input_image


image_data, reference_data = read_image_map(DEBUG_ROAD_IMAGE)
model_mesh, model_normal = construct_drop_model(DEBUG_DROP_DATA, 150, 150, 40, 0.15)
projected_normal_data = project_normal_data_image_space(model_mesh, model_normal)
output_image = create_rain_drop_on_image(image_data, reference_data, projected_normal_data)


cv2.imshow('output_img', output_image)
cv2.waitKey(-1)