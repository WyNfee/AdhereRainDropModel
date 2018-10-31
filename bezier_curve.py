import numpy as np
from scipy.misc import comb as combination


def _quadratic_bezier_point(iterator, control_points):
    """
    return an output point according to iterator and control points of bezier curve
    :param iterator: the iteration of bezier curve definition, start from [0, 1]
    :param control_points: the points of bezier curve definition, formation is (x, y)
    :return: the bezier curve point at current input, formation is (x, y)
    """
    if iterator < 0 or iterator > 1:
        print("Iteration is not correct, quit")
        return 0, 0

    if len(control_points) < 3:
        print("Control points is not correct, expecting at least 3 (only first 3 control points take effect)")
        return 0, 0

    p1_x = control_points[0][0]
    p1_y = control_points[0][1]

    p2_x = control_points[1][0]
    p2_y = control_points[1][1]

    p3_x = control_points[2][0]
    p3_y = control_points[2][1]

    o_x = p1_x * pow(1 - iterator, 2) + 2 * p2_x * (1 - iterator) * iterator + p3_x * pow(iterator, 2)
    o_y = p1_y * pow(1 - iterator, 2) + 2 * p2_y * (1 - iterator) * iterator + p3_y * pow(iterator, 2)

    return o_x, o_y


def _cubic_bezier_point(iterator, control_points):
    """
    return an output point according to iterator and control points of bezier curve
    :param iterator: the iteration of bezier curve definition, start from [0, 1]
    :param control_points: the points of bezier curve definition, formation is (x, y)
    :return: the bezier curve point at current input, formation is (x, y)
    """
    if iterator < 0 or iterator > 1:
        print("Iteration is not correct, quit")
        return 0, 0

    if len(control_points) < 4:
        print("Control points is not correct, expecting at least 4 (only first 4 control points take effect)")
        return 0, 0

    p1_x = control_points[0][0]
    p1_y = control_points[0][1]

    p2_x = control_points[1][0]
    p2_y = control_points[1][1]

    p3_x = control_points[2][0]
    p3_y = control_points[2][1]

    p4_x = control_points[3][0]
    p4_y = control_points[3][1]

    o_x = p1_x * pow(1 - iterator, 3) + 3 * p2_x * pow(1 - iterator, 2) * iterator + 3 * p3_x * pow(iterator, 2) * (1 - iterator) + p4_x * pow(iterator, 3)
    o_y = p1_y * pow(1 - iterator, 3) + 3 * p2_y * pow(1 - iterator, 2) * iterator + 3 * p3_y * pow(iterator, 2) * (1 - iterator) + p4_y * pow(iterator, 3)

    return o_x, o_y


def _generic_bezier_curve(iterator, control_points):
    output_x = 0
    output_y = 0

    combinatin_factor = len(control_points) - 1

    for idx, point in enumerate(control_points):
        x = point[0]
        y = point[1]

        output_x = output_x + combination(combinatin_factor, idx) * x * pow(1 - iterator, combinatin_factor - idx) * pow(iterator, idx)
        output_y = output_y + combination(combinatin_factor, idx) * y * pow(1 - iterator, combinatin_factor - idx) * pow(iterator, idx)

    return output_x, output_y


def _compute_normal_vector_by_points(bezier_point_list_x, bezier_point_list_y):
    """
    compute normal vector by points, only normal vector corresponding to adjacent points, but no
    :param bezier_point_list_x: the points along x axis
    :param bezier_point_list_y:
    :return:
    """
    normal_vector_x_list = list()
    normal_vector_y_list = list()

    for idx in range(len(bezier_point_list_x)):
        if idx == 0:
            continue

        dx = bezier_point_list_x[idx] - bezier_point_list_x[idx - 1]
        dy = bezier_point_list_y[idx] - bezier_point_list_y[idx - 1]

        normal_vec_x = np.cos(np.pi / 2) * dx - np.sin(np.pi / 2) * dy  # x2 = cos(β) x1 − sin(β) y1
        normal_vec_y = np.sin(np.pi / 2) * dx + np.cos(np.pi / 2) * dy  # y2 = sin(β) x1 + cos(β) y1

        normal_vector_x_list.append(normal_vec_x)
        normal_vector_y_list.append(normal_vec_y)

    return normal_vector_x_list, normal_vector_y_list


def generate_bezier_curve_with_normal(control_points, incremental=0.001):
    """
    generate the bezier curve according to control points
    :param control_points: the control points of bezier curve, 1st is start point, last is
    :param incremental: the incremental t parameter of bezier curve
    :return: points of bezier curves, normal of bezier curves
    """
    points_x_list = list()
    points_y_list = list()

    if len(control_points) == 3:
        bezier_func = _quadratic_bezier_point
    else:
        bezier_func = _cubic_bezier_point

    iterator = 0
    # draw bezier curve
    while iterator <= 1:
        o_x, o_y = bezier_func(iterator, control_points)
        points_x_list.append(o_x)
        points_y_list.append(o_y)

        iterator += incremental

    normal_vec_x, normal_vec_y = _compute_normal_vector_by_points(points_x_list, points_y_list)

    return points_x_list, points_y_list, normal_vec_x, normal_vec_y


def generate_bezier_curve(control_points, incremental=0.001):
    """
    generate the bezier curve according to control points
    :param control_points: the control points of bezier curve, 1st is start point, last is
    :param incremental: the incremental t parameter of bezier curve
    :return: points of bezier curves, normal of bezier curves
    """
    points_x_list = list()
    points_y_list = list()

    iterator = 0
    # draw bezier curve
    while iterator <= 1:
        o_x, o_y = _generic_bezier_curve(iterator, control_points)
        points_x_list.append(o_x)
        points_y_list.append(o_y)

        iterator += incremental

    points_x_list.append(control_points[-1][0])
    points_y_list.append(control_points[-1][1])

    return points_x_list, points_y_list


def sample_bezier_curve(control_points, sample_strength):
    """
    generate the bezier curve according to control points
    :param control_points: the control points of bezier curve, 1st is start point, last is
    :param sample_strength: the sample strength of current curve
    :return: points of bezier curves, normal of bezier curves
    """
    points_x_list = list()
    points_y_list = list()

    xs = np.linspace(0, 1, sample_strength)

    if len(control_points) == 3:
        bezier_func = _quadratic_bezier_point
    elif len(control_points) == 4:
        bezier_func = _cubic_bezier_point
    else:
        bezier_func = _generic_bezier_curve

    # draw bezier curve
    for x in xs:
        o_x, o_y = bezier_func(x, control_points)
        points_x_list.append(o_x)
        points_y_list.append(o_y)

    return points_x_list, points_y_list


def compute_normal_positions(point_x_list, point_y_list, normal_x_list, normal_y_list):

    normal_vector_position = list()
    for idx in range(len(point_x_list)):
        # will not process the first one
        if idx == 0:
            continue

        normal_attach_x = (point_x_list[idx] + point_x_list[idx - 1]) / 2.
        normal_attach_y = (point_y_list[idx] + point_y_list[idx - 1]) / 2.

        normal_end_x = normal_attach_x + normal_x_list[idx-1]
        normal_end_y = normal_attach_y + normal_y_list[idx-1]

        # element stored as [ [start_x, end_x], [start_y, end_y] ]
        normal_vector_position.append(((normal_attach_x, normal_end_x), (normal_attach_y, normal_end_y)))

    return normal_vector_position
