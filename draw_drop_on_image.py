"""
Author: Yifei Wang
Date: 2018-10-31
Description: Draw the water drop on image
"""

import cv2
import random
import numpy as np
import construct_drop_mesh as mesh_builder
import copy

DEBUG_ROAD_IMAGE = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\real.jpg"

DEBUG_DROP_LOCATION = (400, 300)

BACKGROUND_BLUR_KERNEL_RANGE = (20, 50)

BOUNDARY_BLUR_KERNEL_RANGE = (10, 40)


def read_processing_image(image_file_path):
    """
    read the image file as numpy array to process
    :param image_file_path: the image pass
    :return: the numpy array of the image, and a reference image to process on
    """
    process_image = cv2.imread(image_file_path)
    reference_image = cv2.imread(image_file_path)
    gbk = int(random.uniform(BACKGROUND_BLUR_KERNEL_RANGE[0], BACKGROUND_BLUR_KERNEL_RANGE[1])) * 2 + 1
    reference_image = cv2.GaussianBlur(reference_image, (gbk, gbk), 0)
    reference_image = np.flipud(reference_image)

    return process_image, reference_image


def project_drop_model_to_image_space(mesh_data, normal):
    """
    convert the model to image space, each index with non-zero value means this point contains
    :param mesh_data:
    :param normal:
    :return:
    """
    height = mesh_data.shape[0]
    width = mesh_data.shape[1]
    channel = mesh_data.shape[2]

    max_x = int(np.ceil(np.max(mesh_data[:, :, 0])))
    max_y = int(np.ceil(np.max(mesh_data[:, :, 1])))

    project_data = np.zeros([max_y, max_x, channel])
    project_data_mask = np.ones([max_y, max_x])

    for h_idx in range(height):
        for w_idx in range(width):
            point_data = mesh_data[h_idx][w_idx]
            pt_x = int(np.floor(point_data[0]))
            pt_y = int(np.floor(point_data[1]))

            project_y_idx = pt_y
            project_x_idx = pt_x

            if project_data_mask[project_y_idx][project_x_idx] == 1:  # note it is initialized with 1
                project_data[project_y_idx][project_x_idx] = normal[h_idx][w_idx]
                project_data_mask[project_y_idx][project_x_idx] = 0

    return project_data


def create_rain_drop_on_image(input_image, reference_image, projected_normal_data, drop_location):
    water_drop_paste = copy.deepcopy(input_image)

    height = projected_normal_data.shape[0]
    width = projected_normal_data.shape[1]

    water_drop_paste_height = water_drop_paste.shape[0]
    water_drop_paste_width = water_drop_paste.shape[1]

    water_drop_mask = np.zeros([water_drop_paste_height, water_drop_paste_width])

    max_pixel_loc_x = 0
    min_pixel_loc_x = width

    max_pixel_loc_y = 0
    min_pixel_loc_y = height

    for h in range(height):
        for w in range(width):
            if projected_normal_data[h][w][0] != 0 and projected_normal_data[h][w][1] != 0 and projected_normal_data[h][w][2] != 0:
                pixel_loc_x = int(drop_location[1]) + w
                pixel_loc_y = int(drop_location[0]) + h

                if pixel_loc_x < min_pixel_loc_x:
                    min_pixel_loc_x = pixel_loc_x

                if pixel_loc_x > max_pixel_loc_x:
                    max_pixel_loc_x = pixel_loc_x

                if pixel_loc_y < min_pixel_loc_y:
                    min_pixel_loc_y = pixel_loc_y

                if pixel_loc_y > max_pixel_loc_y:
                    max_pixel_loc_y = pixel_loc_y

                normal_x = projected_normal_data[h][w][0]
                normal_y = projected_normal_data[h][w][1]

                if normal_x < 0:
                    ref_pick_x = (1 + normal_x) * pixel_loc_x
                else:
                    ref_pick_x = (water_drop_paste_width - pixel_loc_x) * normal_x + pixel_loc_x

                if normal_y < 0:
                    ref_pick_y = -(water_drop_paste_height - pixel_loc_y) * normal_y + pixel_loc_y
                else:
                    ref_pick_y = pixel_loc_y - normal_y * pixel_loc_y

                ref_pick_x = int(min([max([0, ref_pick_x]), water_drop_paste_width]))
                ref_pick_y = int(min([max([0, ref_pick_y]), water_drop_paste_height]))

                reference_value = reference_image[ref_pick_y][ref_pick_x]
                current_value = input_image[pixel_loc_y][pixel_loc_x]
                normal_z = projected_normal_data[h][w][2]
                #TODO: random this 0.3
                translate_factor = normal_z * 0.3

                drop_pixel = current_value * translate_factor + (1 - translate_factor) * reference_value

                water_drop_paste[pixel_loc_y][pixel_loc_x] = drop_pixel
                water_drop_mask[pixel_loc_y][pixel_loc_x] = 1

    gbk = int(random.uniform(BOUNDARY_BLUR_KERNEL_RANGE[0], BOUNDARY_BLUR_KERNEL_RANGE[1])) * 2 + 1
    water_drop_paste = cv2.GaussianBlur(water_drop_paste, (gbk, gbk), 0)

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

            input_image[h][w] = fill_value

    return input_image, water_drop_mask


"""
data_x, data_y, data_ph, data_ps = mesh_builder.load_sample_data(
    mesh_builder.FILE_OUT_SHAPE_X, mesh_builder.FILE_OUT_SHAPE_Y,
    mesh_builder.FILE_PEAK_HEIGHT, mesh_builder.FILE_PEAK_SHAPE)
selected_index = mesh_builder._random_generate_pick_index(data_x)
model_data = mesh_builder._abstract_sample_data(data_x[selected_index], data_y[selected_index], data_ph[selected_index], data_ps[selected_index],
                                                150, 200, 30, 0.1,
                                                enable_debugging=False)
normal_data = mesh_builder.compute_normal_based_on_mesh(model_data, enable_debugging=False)

"""
#np.save(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\mesh", model_data)
#np.save(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\normal", normal_data)


load_model_data = np.load(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\mesh.npy")
load_normal_data = np.load(r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\Data\saved\normal.npy")

image_data, ref_image_data = read_processing_image(DEBUG_ROAD_IMAGE)
project_normal = project_drop_model_to_image_space(load_model_data, load_normal_data)
# TODO: generate the mask
output_image, water_drop_mask = create_rain_drop_on_image(image_data, ref_image_data, project_normal, DEBUG_DROP_LOCATION)

cv2.imshow('output_img', output_image)
cv2.imshow('mask', water_drop_mask)
cv2.waitKey(-1)
