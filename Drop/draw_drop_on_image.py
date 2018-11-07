"""
Author: Yifei Wang
Date: 2018-10-31
Description: Draw the water drop on image
"""

import cv2
import random
import numpy as np
from Drop import construct_drop_mesh as mesh_builder
import copy

BACKGROUND_BLUR_KERNEL_RANGE = (5, 15)

BOUNDARY_BLUR_KERNEL_RANGE = (18, 25)

BLEND_RANGE = (0.75, 0.95)

WATER_DROP_AMOUNT_RANGE = (1, 5)

WATER_DROP_SIZE_RANGE = (60, 300)

WATER_DROP_SHAPE_OFFSET_RANGE = (0.4, 0.8)

WATER_DROP_HEIGHT_RANGE = (60, 200)

WATER_DROP_LOCATION_X = (0, 1000)

WATER_DROP_LOCATION_Y = (0, 550)

LIGHT_INTENSITY_RANGE = (1.1, 1.5)

DROP_MAX_TRIAL = 200


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
    process_image = process_image[77:-3, :]
    reference_image = cv2.imread(image_file_path)
    reference_image = reference_image[77:-3, :]
    gbk = int(random.uniform(BACKGROUND_BLUR_KERNEL_RANGE[0], BACKGROUND_BLUR_KERNEL_RANGE[1])) * 2 + 1
    # print("reference kernel size %d" % gbk)
    reference_image = cv2.GaussianBlur(reference_image, (gbk, gbk), 0)
    reference_image = np.flipud(reference_image)
    # the reason of multiple light intensity (should larger than 1)
    # that is the lux in world is much larger than those in photo graphs
    light_intensity = int(random.uniform(LIGHT_INTENSITY_RANGE[0], LIGHT_INTENSITY_RANGE[1]))
    reference_image = reference_image * light_intensity
    reference_image = np.clip(reference_image, 0, 255).astype(np.uint8)

    return process_image, reference_image


def project_drop_model_to_image_space(mesh_data):
    """
    convert the model to image space...
    the model coordinates value maps to image row anc column index
    :param mesh_data: the mesh data
    :return: the projected mesh
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
                project_data[project_y_idx][project_x_idx] = mesh_data[h_idx][w_idx]
                project_data_mask[project_y_idx][project_x_idx] = 0

    return project_data


def create_rain_drop_on_image(output_image, drop_mask, input_image, reference_image, projected_normal_data, drop_location, blend_range):
    """
    using the projected normal and reference image,
    to draw the water drop effect on image we interested in.
    :param output_image: the image to output
    :param drop_mask: the drop mask to work on
    :param input_image: the source image we are working on
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

    # print("project height %d, width %d" % (height, width))

    # print("drop location y %d, x %d" % (drop_location[0], drop_location[1]))
    max_pixel_loc_x = 0
    min_pixel_loc_x = water_drop_paste_width

    max_pixel_loc_y = 0
    min_pixel_loc_y = water_drop_paste_height

    # loop on projected normal data, using the normal data to map the reference image, shows on water drop refraction
    for h in range(height):
        for w in range(width):
            # if the normal is [0,0,0]. means the ones not mapped on image, ignore it
            if projected_normal_data[h][w][0] != 0 and projected_normal_data[h][w][1] != 0 and projected_normal_data[h][w][2] != 0:

                # get where the pixel on image we need to draw
                pixel_loc_x = int(drop_location[1]) + w
                pixel_loc_y = int(drop_location[0]) + h

                # do boundary protection
                if pixel_loc_x >= water_drop_paste_width:
                    continue

                if pixel_loc_y >= water_drop_paste_height:
                    continue

                # record the min and max position of drop created
                if pixel_loc_x < min_pixel_loc_x:
                    min_pixel_loc_x = pixel_loc_x

                if pixel_loc_x > max_pixel_loc_x:
                    max_pixel_loc_x = pixel_loc_x

                if pixel_loc_y < min_pixel_loc_y:
                    min_pixel_loc_y = pixel_loc_y

                if pixel_loc_y > max_pixel_loc_y:
                    max_pixel_loc_y = pixel_loc_y

                # get the normal x, y information
                normal_x = -projected_normal_data[h][w][0]
                normal_y = -projected_normal_data[h][w][1]

                # as current image center, finding out the pixel of current drop should mapping on reference image
                offset_x = abs((water_drop_paste_width / 2 - pixel_loc_x) * normal_x)
                offset_y = abs((water_drop_paste_height / 2 - pixel_loc_y) * normal_y)
                if normal_x < 0:
                    # ref_pick_x = (water_drop_paste_width - pixel_loc_x) * normal_x + pixel_loc_x
                    ref_pick_x = pixel_loc_x - offset_x
                else:
                    # ref_pick_x = (1 - normal_x) * pixel_loc_x
                    ref_pick_x = pixel_loc_x + offset_x

                if normal_y < 0:
                    # ref_pick_y = pixel_loc_y + normal_y * pixel_loc_y
                    ref_pick_y = pixel_loc_y + offset_y
                else:
                    # ref_pick_y = (water_drop_paste_height - pixel_loc_y) * normal_y + pixel_loc_y
                    ref_pick_y = pixel_loc_y - offset_y

                ref_pick_x = int(min([max([0, ref_pick_x]), water_drop_paste_width-1]))
                ref_pick_y = int(min([max([0, ref_pick_y]), water_drop_paste_height-1]))

                reference_value = reference_image[ref_pick_y][ref_pick_x]
                current_value = input_image[pixel_loc_y][pixel_loc_x]
                normal_z = abs(projected_normal_data[h][w][2])

                # using normal z as factor, to measure how to blend source image and reference image
                blend_factor = random.uniform(blend_range[0], blend_range[1])

                # if z value is small, mean x or y is strong, then use reference value more
                # other wise, use current value more
                # translate_factor = normal_z  # * blend_factor
                drop_pixel = reference_value * (1 - normal_z) * blend_factor + current_value * normal_z
                drop_pixel = np.clip(drop_pixel, 0, 255)

                # apply the drop pixel and record the mask of the image
                water_drop_paste[pixel_loc_y][pixel_loc_x] = drop_pixel
                drop_mask[pixel_loc_y][pixel_loc_x] = 1

    # create a gaussian blurry image, to reduce the edge sharpen
    gbk = int(random.uniform(BOUNDARY_BLUR_KERNEL_RANGE[0], BOUNDARY_BLUR_KERNEL_RANGE[1])) * 2 + 1
    # print("boundary blur factor %d" % gbk)

    water_drop_paste = cv2.GaussianBlur(water_drop_paste, (gbk, gbk), 0)

    # print("min y %d, min x %d" % (min_pixel_loc_y, min_pixel_loc_x))
    # print("max y %d, max x %d" % (max_pixel_loc_y, max_pixel_loc_x))

    # create a filtering matrix, a pyramid like, the edge is close to 0,
    # and center is close to 1, as a blend mask

    fill_area_height = max_pixel_loc_y - min_pixel_loc_y
    fill_area_width = max_pixel_loc_x - min_pixel_loc_x

    min_pixel_loc_y = min_pixel_loc_y - int(fill_area_height / 2)
    min_pixel_loc_x = min_pixel_loc_x - int(fill_area_width / 2)

    max_pixel_loc_y = max_pixel_loc_y + int(fill_area_height / 2)
    max_pixel_loc_x = max_pixel_loc_x + int(fill_area_width / 2)

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
    # TODO: think about using Bi_Gaussian Distribution replace this pyramid
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

            # do blend for this
            current_value = input_image[h][w]
            reference_value = water_drop_paste[h][w]
            mask_filter_value = fill_area[h - min_pixel_loc_y][w - min_pixel_loc_x]
            mask_filter_value = mask_filter_value ** 1.5

            fill_value = reference_value * mask_filter_value + (1 - mask_filter_value) * current_value

            output_image[h][w] = fill_value  # (int(255 * mask_filter_value), 0, int(255 * mask_filter_value))  # for debug

    return output_image, drop_mask


def rotate_bound(image, angle):
    # grab the dimensions of the image and then determine the
    # center
    (h, w) = image.shape[:2]
    (cX, cY) = (w // 2, h // 2)

    # grab the rotation matrix (applying the negative of the
    # angle to rotate clockwise), then grab the sine and cosine
    # (i.e., the rotation components of the matrix)
    M = cv2.getRotationMatrix2D((cX, cY), -angle, 1.0)
    cos = np.abs(M[0, 0])
    sin = np.abs(M[0, 1])

    # compute the new bounding dimensions of the image
    nW = int((h * sin) + (w * cos))
    nH = int((h * cos) + (w * sin))

    # adjust the rotation matrix to take into account translation
    M[0, 2] += (nW / 2) - cX
    M[1, 2] += (nH / 2) - cY

    # perform the actual rotation and return the image
    return cv2.warpAffine(image, M, (nW, nH))


def create_water_drops_on_image(output_image, drop_mask, image_data, reference_image, data_x, data_y, data_ph, data_ps):
    water_drop_amount = int(random.uniform(WATER_DROP_AMOUNT_RANGE[0], WATER_DROP_AMOUNT_RANGE[1]))
    # print("Generating %d drops on image" % water_drop_amount)
    water_drop_idx = 0

    water_drop_location_list = list()
    water_drop_parameter_list = list()

    print("About to generate %d drops on image" % water_drop_amount)
    trial_counter = 0
    while water_drop_idx < water_drop_amount and trial_counter < DROP_MAX_TRIAL:
        water_drop_size_x = int(random.uniform(WATER_DROP_SIZE_RANGE[0], WATER_DROP_SIZE_RANGE[1]))
        water_drop_size_y = int(random.uniform(WATER_DROP_SIZE_RANGE[0], WATER_DROP_SIZE_RANGE[1]))

        water_drop_height = int(random.uniform(WATER_DROP_HEIGHT_RANGE[0], WATER_DROP_HEIGHT_RANGE[1]))

        water_drop_shape = random.uniform(WATER_DROP_SHAPE_OFFSET_RANGE[0], WATER_DROP_SHAPE_OFFSET_RANGE[1])
        water_drop_loc_x = int(random.uniform(WATER_DROP_LOCATION_X[0], WATER_DROP_LOCATION_X[1]))
        water_drop_loc_y = int(random.uniform(WATER_DROP_LOCATION_Y[0], WATER_DROP_LOCATION_Y[1]))

        # roughly estimate whether this place is occupied or not
        # since we are about to rotate the drop at the place, reserve the largest space for it...
        # not ideal...
        drop_xmin = water_drop_loc_x
        drop_ymin = water_drop_loc_y
        drop_xmax = water_drop_loc_x + water_drop_size_x
        drop_ymax = water_drop_loc_y + water_drop_size_y

        drop_x_center = (drop_xmax + drop_xmin) / 2
        drop_y_center = (drop_ymax + drop_ymin) / 2
        drop_side = max([water_drop_size_y, water_drop_size_x])
        drop_side = drop_side * 2

        drop_test_xmin = drop_x_center - drop_side / 2
        drop_test_xmax = drop_x_center + drop_side / 2
        drop_test_ymin = drop_y_center - drop_side / 2
        drop_test_ymax = drop_y_center + drop_side / 2

        is_valid_trial = True

        if len(water_drop_location_list) == 0:
            water_drop_location_list.append((drop_xmin, drop_ymin, drop_xmax, drop_ymax))
            print("%.4f, %.4f, %.4f, %.4f" % (drop_xmin, drop_ymin, drop_xmax, drop_ymax))
            water_drop_parameter_list.append((water_drop_size_x, water_drop_size_y, water_drop_height, water_drop_shape, water_drop_loc_x, water_drop_loc_y))
            water_drop_idx += 1
            trial_counter += 1
            continue
        else:
            for loc in water_drop_location_list:
                loc_xmin = loc[0]
                loc_ymin = loc[1]
                loc_xmax = loc[2]
                loc_ymax = loc[3]

                loc_x_center = (loc_xmin + loc_xmax) / 2
                loc_y_center = (loc_ymin + loc_ymax) / 2
                loc_side = max([loc_ymax - loc_ymin, loc_xmax - loc_xmin])
                loc_side = loc_side * 2

                loc_test_xmin = loc_x_center - loc_side / 2
                loc_test_xmax = loc_x_center + loc_side / 2
                loc_test_ymin = loc_y_center - loc_side / 2
                loc_test_ymax = loc_y_center + loc_side / 2

                xmin = max([drop_test_xmin, loc_test_xmin])
                xmax = min([drop_test_xmax, loc_test_xmax])
                ymin = max([drop_test_ymin, loc_test_ymin])
                ymax = min([drop_test_ymax, loc_test_ymax])

                if xmax <= xmin and ymax <= ymin:
                    continue
                else:
                    is_valid_trial = False
                    break

        if is_valid_trial is True:
            water_drop_location_list.append((drop_xmin, drop_ymin, drop_xmax, drop_ymax))
            print("%.4f, %.4f, %.4f, %.4f" % (drop_xmin, drop_ymin, drop_xmax, drop_ymax))
            water_drop_parameter_list.append((water_drop_size_x, water_drop_size_y, water_drop_height, water_drop_shape, water_drop_loc_x, water_drop_loc_y))
            water_drop_idx += 1

        trial_counter += 1

    print("Total will generate %d" % len(water_drop_parameter_list))
    for water_drop_idx, parameter in enumerate(water_drop_parameter_list):
        blend_range = BLEND_RANGE

        selected_index = mesh_builder._random_generate_pick_index(data_x)
        model_data = mesh_builder._abstract_sample_data(data_x[selected_index], data_y[selected_index], data_ph[selected_index], data_ps[selected_index],
                                                        parameter[1], parameter[0], parameter[2], parameter[3],
                                                        enable_debugging=False)

        project_mesh = project_drop_model_to_image_space(model_data)
        project_normal = mesh_builder.compute_normal_based_on_mesh(project_mesh, enable_debugging=False)
        # random rotation
        rotate_degree = int(random.uniform(0, 360))
        rotated_normal = rotate_bound(project_normal, rotate_degree)
        output_image, drop_mask = create_rain_drop_on_image(output_image, drop_mask,
                                                            image_data, reference_image, rotated_normal,
                                                            (parameter[5], parameter[4]), blend_range)

        print("Drop %d has been generated" % water_drop_idx)

    return output_image, drop_mask


def generate_image_with_water_drop(image_file_path):
    data_x, data_y, data_ph, data_ps = mesh_builder.load_sample_data(
        mesh_builder.FILE_OUT_SHAPE_X, mesh_builder.FILE_OUT_SHAPE_Y,
        mesh_builder.FILE_PEAK_HEIGHT, mesh_builder.FILE_PEAK_SHAPE)

    origin_image, ref_image = read_processing_image(image_file_path)
    output_image = copy.deepcopy(origin_image)
    drop_mask = np.zeros([output_image.shape[0], output_image.shape[1]])
    output_image, drop_mask = create_water_drops_on_image(output_image, drop_mask, origin_image, ref_image, data_x, data_y, data_ph, data_ps)

    # cv2.imshow('out_image', output_image)
    # cv2.imshow('mask', drop_mask)
    # cv2.waitKey(-1)

    return output_image, drop_mask


# generate_image_with_water_drop(r"D:\Data\TFTrain\AdhereRainDrop\Raw\20170202_1621570093.bmp")
