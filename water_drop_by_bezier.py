import bezier_curve
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

RAIN_DROP_MODEL_SIZE = 50

BEZIER_CURVE_CONTROL_POINTS_BASE_HEIGHT = [(-RAIN_DROP_MODEL_SIZE / 2, 0), (-15, 32), (0, 70), (15, 32), (RAIN_DROP_MODEL_SIZE / 2, 0)]
BEZIER_CURVE_CONTROL_POINTS_SHAPE_1 = [(-RAIN_DROP_MODEL_SIZE / 2, 0), (-30, 30), (0, 0), (30, 30), (RAIN_DROP_MODEL_SIZE / 2, 0)]
BEZIER_CURVE_CONTROL_POINTS_SHAPE_2 = [(-RAIN_DROP_MODEL_SIZE / 2, 0), (-30, -30), (30, -30), (RAIN_DROP_MODEL_SIZE / 2, 0)]
BEZIER_CURVE_CONTROL_POINTS_BASE_OFFSET = [(-RAIN_DROP_MODEL_SIZE / 2, 0), (-15, 0), (0, 70), (35, 0), (RAIN_DROP_MODEL_SIZE / 2, 0)]


def _get_curve_line(control_points):
    axis_1_points, axis_2_points = bezier_curve.generate_bezier_curve(control_points)
    return axis_1_points, axis_2_points


def convert_point_coord_to_index(point_value):
    if point_value < 0:
        point_idx = np.floor(point_value)
    else:
        point_idx = np.ceil(point_value)

    return point_idx


def generate_rain_drop_model_data():
    bys_1, bzs_1 = _get_curve_line(BEZIER_CURVE_CONTROL_POINTS_BASE_HEIGHT)
    bys_2, bxs_2 = _get_curve_line(BEZIER_CURVE_CONTROL_POINTS_BASE_OFFSET)
    bxs_1 = bxs_2

    sys_1, sxs_1 = _get_curve_line(BEZIER_CURVE_CONTROL_POINTS_SHAPE_1)
    szs_1 = [0] * len(sys_1)
    min_x_1 = min(sxs_1)
    min_y_1 = min(sys_1)
    max_x_1 = max(sxs_1)
    max_y_1 = max(sys_1)
    sys_2, sxs_2 = _get_curve_line(BEZIER_CURVE_CONTROL_POINTS_SHAPE_2)
    szs_2 = [0] * len(sys_2)
    min_x_2 = min(sxs_2)
    min_y_2 = min(sys_2)
    max_x_2 = max(sxs_1)
    max_y_2 = max(sys_1)

    min_x = min(min_x_1, min_x_2)
    min_y = min(min_y_1, min_y_2)
    max_x = max(max_x_1, max_x_2)
    max_y = max(max_y_1, max_y_2)

    min_x_idx = convert_point_coord_to_index(min_x)
    max_x_idx = convert_point_coord_to_index(max_x)
    min_y_idx = convert_point_coord_to_index(min_y)
    max_y_idx = convert_point_coord_to_index(max_y)

    width_range = int(max_x_idx - min_x_idx)
    height_range = int(max_y_idx - min_y_idx)

    model_data = list()

    for m_idx in range(len(sys_1)):
        start_point = (sxs_1[m_idx], szs_1[m_idx])
        middle_point = (bxs_1[m_idx], bzs_1[m_idx])
        end_point = (sxs_2[m_idx], szs_2[m_idx])

        control_points = (start_point, middle_point, end_point)
        sur_x, sur_z = _get_curve_line(control_points)
        sur_y = [sys_1[m_idx]] * len(sur_x)
        sur_x = [x - min_x for x in sur_x]
        sur_y = [y - min_y for y in sur_y]

        model_data.append((sur_x, sur_y, sur_z))

    return model_data, width_range, height_range


def convert_model_to_mesh(model_data, width, height):
    sample_step_x = int(len(model_data[0][0]) / width)
    sample_step_y = int(len(model_data) / height)

    # reserve 1 slot for boundary
    mesh_data_height = height + 1
    mesh_data_width = width + 1
    mesh_data = np.zeros([mesh_data_height, mesh_data_width, 3])

    for h_idx in range(len(model_data)):
        if h_idx % sample_step_y == 0:
            sample_h_idx = int(h_idx / sample_step_y)

            for w_idx in range(len(model_data[0][0])):
                if w_idx % sample_step_x == 0:
                    sample_w_idx = int(w_idx / sample_step_x)

                    mesh_point_x = model_data[h_idx][0][w_idx]
                    mesh_point_y = model_data[h_idx][1][w_idx]
                    mesh_point_z = model_data[h_idx][2][w_idx]

                    if sample_h_idx < mesh_data_height and sample_w_idx < mesh_data_width:
                        mesh_data[sample_h_idx][sample_w_idx][0] = mesh_point_x
                        mesh_data[sample_h_idx][sample_w_idx][1] = mesh_point_y
                        if sample_w_idx == mesh_data_width - 1:
                            mesh_data[sample_h_idx][sample_w_idx][2] = 0
                        else:
                            mesh_data[sample_h_idx][sample_w_idx][2] = mesh_point_z

    # adding the last point, high probability we will miss it
    for w_idx in range(len(model_data[0][0])):
        sample_w_idx = int(w_idx / sample_step_x)

        mesh_point_x = model_data[-1][0][w_idx]
        mesh_point_y = model_data[-1][1][w_idx]
        mesh_point_z = model_data[-1][2][w_idx]

        if sample_w_idx < mesh_data_width:
            mesh_data[-1][sample_w_idx][0] = mesh_point_x
            mesh_data[-1][sample_w_idx][1] = mesh_point_y
            if sample_w_idx == mesh_data_width - 1:
                mesh_data[sample_h_idx][sample_w_idx][2] = 0
            else:
                mesh_data[sample_h_idx][sample_w_idx][2] = mesh_point_z

    return mesh_data


def _normalize_vector(input_vector):
    norm = np.linalg.norm(input_vector)
    norm_vec = input_vector / (norm + 1e-6)
    return norm_vec


def compute_normal(mesh_data, current_h_idx, current_w_idx):

    current_pt = mesh_data[current_h_idx][current_w_idx]
    left_pt = mesh_data[current_h_idx][current_w_idx - 1]
    right_pt = mesh_data[current_h_idx][current_w_idx + 1]
    bottom_pt = mesh_data[current_h_idx + 1][current_w_idx]
    up_pt = mesh_data[current_h_idx - 1][current_w_idx]

    left_vector = current_pt - left_pt
    right_vector = right_pt - current_pt
    up_vector = current_pt - up_pt
    bottom_vector = bottom_pt - current_pt

    left_up_normal = -np.cross(left_vector, up_vector)
    left_bottom_normal = -np.cross(left_vector, bottom_vector)
    right_up_normal = -np.cross(right_vector, up_vector)
    right_bottom_normal = -np.cross(right_vector, bottom_vector)

    left_up_normal = _normalize_vector(left_up_normal)
    left_bottom_normal = _normalize_vector(left_bottom_normal)
    right_up_normal = _normalize_vector(right_up_normal)
    right_bottom_normal = _normalize_vector(right_bottom_normal)

    final_normal = (left_up_normal + left_bottom_normal + right_up_normal + right_bottom_normal) / 4

    final_normal = _normalize_vector(final_normal)

    return final_normal


def compute_normal_based_on_mesh(mesh_data):
    height = mesh_data.shape[0]
    width = mesh_data.shape[1]

    normal_data = np.zeros([height, width, 3])

    for h_idx in range(height):
        for w_idx in range(width):
            if 0 < h_idx < height-1 and 0 < w_idx < width-1:
                norm = compute_normal(mesh_data, h_idx, w_idx)
                normal_data[h_idx][w_idx] = norm

    return normal_data


rain_drop_model, range_x, range_y = generate_rain_drop_model_data()
rain_drop_mesh = convert_model_to_mesh(rain_drop_model, range_x, range_y)
rain_drop_normal = compute_normal_based_on_mesh(rain_drop_mesh)

fig = plt.figure()
ax = fig.gca(projection='3d')
ax.set_title("rain drop model Model")
ax.set_xlabel('X axis')
ax.set_ylabel('Y axis')
ax.set_zlabel('Z axis')
ax.set_xlim(0, RAIN_DROP_MODEL_SIZE)
ax.set_ylim(0, RAIN_DROP_MODEL_SIZE)
ax.set_zlim(0, RAIN_DROP_MODEL_SIZE)
for idx, data in enumerate(rain_drop_model):
    if idx % 20 == 0:
        ax.plot(data[0], data[1], data[2], 'b-')


fig = plt.figure()
ax = fig.gca(projection='3d')
ax.set_title("rain drop Mesh - 3D")
ax.set_xlabel('X axis')
ax.set_ylabel('Y axis')
ax.set_zlabel('Z axis')
ax.set_xlim(0, RAIN_DROP_MODEL_SIZE)
ax.set_ylim(0, RAIN_DROP_MODEL_SIZE)
ax.set_zlim(0, RAIN_DROP_MODEL_SIZE)
ax.plot_wireframe(rain_drop_mesh[:, :, 0], rain_drop_mesh[:, :, 1], rain_drop_mesh[:, :, 2], color='b')
for m_h_idx in range(rain_drop_mesh.shape[0]):
    for m_w_idx in range(rain_drop_mesh.shape[1]):
        if m_h_idx % 5 == 0 and m_w_idx % 5 == 0:
            normal = rain_drop_normal[m_h_idx][m_w_idx] * 5
            point = rain_drop_mesh[m_h_idx, m_w_idx]
            ax.quiver(point[0], point[1], point[2], normal[0], normal[1], normal[2], color='r')

plt.show()
