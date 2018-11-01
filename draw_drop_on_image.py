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

BLEND_FACTOR_RANGE = (0.2, 0.8)

WATER_DROP_AMOUNT_RANGE = (3, 10)

WATER_DROP_SIZE_RANGE = (10, 300)

WATER_DROP_SHAPE_OFFSET_RANGE = (0.1, 0.4)

WATER_DROP_HEIGHT_RANGE = (5, 20)


def read_processing_image(image_file_path):
    """
    read the image file as numpy array to process,
    generate a reference image for further process,
    just simulate the blurry the water drop reflection and refraction, camera out-side focus and distortion
    using random gaussian blur to simulate the strength of the whole blurry
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
    convert the model to image space...
    the model coordinates value maps to image row anc column index
    other words, encode the normal into image space indexes
    :param mesh_data: the mesh data
    :param normal: the normal of the mesh data
    :return: the projected normal
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


def create_rain_drop_on_image(input_image, reference_image, projected_normal_data, drop_location, blend_range):
    """
    using the projected normal and reference image,
    to draw the water drop effect on image we interested in.
    :param input_image: the image we are working on
    :param reference_image: the reference image to simulate the distortion and focus causing by light refraction/reflection and camera projection/distortion
    :param projected_normal_data: the normal data projected on image index form
    :param drop_location: the location where to draw the drop
    :param blend_range: the blend range between source and reference image
    :return: a image draw the dropped on, and a mask about the drop
    """
    # the first layer of drawing the water drop, since first drawing on image will causing the drop border alias serious
    water_drop_paste = copy.deepcopy(input_image)

    water_drop_paste_height = water_drop_paste.shape[0]
    water_drop_paste_width = water_drop_paste.shape[1]

    # the projected normal shape
    height = projected_normal_data.shape[0]
    width = projected_normal_data.shape[1]

    water_drop_mask = np.zeros([water_drop_paste_height, water_drop_paste_width])

    max_pixel_loc_x = 0
    min_pixel_loc_x = width

    max_pixel_loc_y = 0
    min_pixel_loc_y = height

    # loop on projected normal data, using the normal data to map the reference image, shows on water drop refraction
    for h in range(height):
        for w in range(width):
            # if the normal is [0,0,0]. means the ones not mapped on image, ignore it
            if projected_normal_data[h][w][0] != 0 and projected_normal_data[h][w][1] != 0 and projected_normal_data[h][w][2] != 0:

                # get where the pixel on image we need to draw
                pixel_loc_x = int(drop_location[1]) + w
                pixel_loc_y = int(drop_location[0]) + h

                # boundary protection
                if pixel_loc_x < min_pixel_loc_x:
                    min_pixel_loc_x = pixel_loc_x

                if pixel_loc_x > max_pixel_loc_x:
                    max_pixel_loc_x = pixel_loc_x

                if pixel_loc_y < min_pixel_loc_y:
                    min_pixel_loc_y = pixel_loc_y

                if pixel_loc_y > max_pixel_loc_y:
                    max_pixel_loc_y = pixel_loc_y

                # get the normal x, y information
                normal_x = projected_normal_data[h][w][0]
                normal_y = projected_normal_data[h][w][1]

                # as current image center, finding out the pixel of current drop should mapping on reference image
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

                # using normal z as factor, to measure how to blend source image and reference image
                blend_factor = random.uniform(BLEND_FACTOR_RANGE)
                translate_factor = normal_z * blend_factor

                drop_pixel = current_value * translate_factor + (1 - translate_factor) * reference_value

                # apply the drop pixel and record the mask of the image
                water_drop_paste[pixel_loc_y][pixel_loc_x] = drop_pixel
                water_drop_mask[pixel_loc_y][pixel_loc_x] = 1

    # create a gaussian blurry image, to reduce the edge sharpen
    gbk = int(random.uniform(BOUNDARY_BLUR_KERNEL_RANGE[0], BOUNDARY_BLUR_KERNEL_RANGE[1])) * 2 + 1
    water_drop_paste = cv2.GaussianBlur(water_drop_paste, (gbk, gbk), 0)

    # create a filtering matrix, a pyramid like, the edge is close to 0,
    # and center is close to 1, as a blend mask
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

    # create the pyramid
    for f_x_idx in range(fill_area_width):
        for f_y_idx in range(fill_area_height):
            x_factor = (half_max_width_index - abs(f_x_idx - half_max_width_index)) / half_max_width_index
            y_factor = (half_max_height_index - abs(f_y_idx - half_max_height_index)) / half_max_height_index
            fill_area[f_y_idx][f_x_idx] = x_factor / 2 + y_factor / 2

    # find out the where to fill the value and use filter to blurry the edge
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


def create_water_drops_on_image(image_data, reference_image, data_x, data_y, data_ph, data_ps):
    water_drop_amount = int(random.uniform(WATER_DROP_AMOUNT_RANGE))

    water_drop_idx = 0

    while water_drop_idx < water_drop_amount:
        water_drop_size_x = int(random.uniform(WATER_DROP_SIZE_RANGE))
        water_drop_size_y = int(random.uniform(WATER_DROP_SIZE_RANGE))
        water_drop_height = int(random.uniform(WATER_DROP_HEIGHT_RANGE))
        water_drop_shape = random.uniform(WATER_DROP_SHAPE_OFFSET_RANGE)

        selected_index = mesh_builder._random_generate_pick_index(data_x)
        model_data = mesh_builder._abstract_sample_data(data_x[selected_index], data_y[selected_index], data_ph[selected_index], data_ps[selected_index],
                                                        water_drop_size_x, water_drop_size_y, water_drop_height, water_drop_shape,
                                                        enable_debugging=False)
        normal_data = mesh_builder.compute_normal_based_on_mesh(model_data, enable_debugging=False)
        project_normal = project_drop_model_to_image_space(model_data, normal_data)
        output_image, mask_image = create_rain_drop_on_image(image_data, reference_image, project_normal, DEBUG_DROP_LOCATION)

        water_drop_idx += 1

    return output_image, mask_image

