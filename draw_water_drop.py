"""
Author: Yifei Wang
Date: 2018-10-24
Description: This script will render water drop on an image
"""

import cv2
import numpy as np
import water_drop_model as model

DEBUG_ROAD_IMAGE = "/home/maxieye/Documents/Git/AdhereRainDropModel/road.jpg"
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


def create_rain_drop_on_image(base_image, reference_image, normal_data):

    height = normal_data.shape[0]
    width = normal_data.shape[1]

    base_image_height = base_image.shape[0]
    base_image_width = base_image.shape[1]

    for h in range(height):
        for w in range(width):
            if normal_data[h][w][0] != 0 and normal_data[h][w][1] != 0 and normal_data[h][w][2] != 0:
                pixel_loc_x = int(base_image_width/2) + w
                pixel_loc_y = int(base_image_height/2) + h
                base_image[pixel_loc_y][pixel_loc_x] = _mapping_normal_to_pixel(reference_image, normal_data[h][w])
    return base_image


image_data, reference_data = read_image_map(DEBUG_ROAD_IMAGE)
model_mesh, model_normal = construct_drop_model(DEBUG_DROP_DATA, 50, 50, 40, 0.15)
projected_normal_data = project_normal_data_image_space(model_mesh, model_normal)
output_image = create_rain_drop_on_image(image_data, reference_data, projected_normal_data)


cv2.imshow('output_img', output_image)
cv2.waitKey(-1)