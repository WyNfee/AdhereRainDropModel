"""
Author: Yifei Wang
Date: 2018-11-15
Description: This script will generate the video
"""

import os
import cv2

# the directory where to store the image
DIR_WORKING_DIR = r"D:\Data\Raw\Rain\Log\jpg"

# the video file to write out
DIR_VALID_DIR = r"D:\Data\TFTrain\AdhereRainDrop\Video\T1.avi"

# generate video frame rate
VIDEO_FRAME_RATE = 25


def _find_valid_file_pair(working_dir):
    all_file_list = os.listdir(working_dir)
    valid_file_list = list()

    for file in all_file_list:
        is_valid_file = file.endswith(".jpg")
        if is_valid_file:
            valid_file_list.append(file)

    return valid_file_list


def _generate_video_from_file_list(working_dir, output_file, file_list, fps):
    fourcc = cv2.VideoWriter_fourcc(*'XVID')
    video_writer = cv2.VideoWriter(output_file, fourcc, fps, (1280, 640), True)

    total_file_amount = len(file_list)

    for idx in range(total_file_amount):
        file_name = file_list[idx]
        file_path = os.path.join(working_dir, file_name)

        image_data = cv2.imread(file_path)
        image_data = image_data[77:-3, :]
        video_writer.write(image_data)

        print("Video Creation Progress %d/%d" % (idx+1, total_file_amount))

    video_writer.release()


valid_list = _find_valid_file_pair(DIR_WORKING_DIR)
_generate_video_from_file_list(DIR_WORKING_DIR, DIR_VALID_DIR, valid_list, VIDEO_FRAME_RATE)
