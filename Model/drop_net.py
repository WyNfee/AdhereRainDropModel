"""
Author: Yifei Wang
Date: 2018-11-01
Description: the model definition of drop net
"""

import tensorflow as tf

slim = tf.contrib.slim


def _def_arg_scope(weight_decay=5e-4, is_training=True):
    batch_norm_params = {'is_training': is_training, 'decay': 0.9,
                         'updates_collections': tf.GraphKeys.UPDATE_OPS}
    with slim.arg_scope(
            [slim.conv2d, slim.fully_connected, slim.conv2d_transpose],
            activation_fn=tf.nn.leaky_relu,
            weights_regularizer=slim.l2_regularizer(weight_decay),
            weights_initializer=tf.contrib.layers.variance_scaling_initializer(),
            biases_initializer=tf.zeros_initializer(),
            normalizer_fn=slim.batch_norm,
            normalizer_params=batch_norm_params):
        with slim.arg_scope(
                [slim.conv2d, slim.max_pool2d],
                padding='SAME') as scope_def:
            return scope_def


def get_net(input_data, weight_decay=5e-4, is_training=True):
    with tf.name_scope('reshape_input'):
        image = tf.reshape(input_data, [-1, 160, 320, 3])

    scope_def = _def_arg_scope(weight_decay, is_training)

    with slim.arg_scope(scope_def):
        with tf.variable_scope('drop_net', 'drop_net', [image], reuse=tf.AUTO_REUSE):

            net = slim.repeat(image, 1, slim.conv2d, 16, [5, 5], scope='conv1')
            net = tf.layers.dropout(net, rate=0.15, training=is_training)
            net = slim.max_pool2d(net, [2, 2], scope='pool1')  # 80 160

            net = slim.repeat(net, 1, slim.conv2d, 24, [5, 5], scope='conv2')
            net = tf.layers.dropout(net, rate=0.15, training=is_training)
            net = slim.max_pool2d(net, [2, 2], scope='pool2')  # 40 80

            net = slim.repeat(net, 2, slim.conv2d, 32, [3, 3], scope='conv3')
            net = tf.layers.dropout(net, rate=0.15, training=is_training)
            net = slim.max_pool2d(net, [2, 2], scope='pool3')  # 20 40

            net = slim.repeat(net, 2, slim.conv2d, 32, [3, 3], scope='conv4')
            net4 = net
            net = tf.layers.dropout(net, rate=0.15, training=is_training)
            net = slim.max_pool2d(net, [2, 2], scope='pool4')  # 10 20

            net = slim.repeat(net, 2, slim.conv2d, 32, [3, 3], scope='conv5')
            net5 = net
            net = tf.layers.dropout(net, rate=0.15, training=is_training)
            net = slim.max_pool2d(net, [2, 2], scope='pool5')  # 5 10

            net = slim.repeat(net, 1, slim.conv2d_transpose, 32, [3, 3], stride=2, scope='deconv5')
            net = net + net5
            net = slim.repeat(net, 1, slim.conv2d_transpose, 32, [3, 3], stride=2, scope='deconv4')
            net = net + net4

            inputs_shape = net.get_shape()
            inputs_rank = inputs_shape.ndims
            norm_dim = tf.range(inputs_rank - 1, inputs_rank)
            norm_net = tf.nn.l2_normalize(net, norm_dim, epsilon=1e-12)

            logits = slim.conv2d(norm_net, 2, [1, 1], activation_fn=None, normalizer_fn=None, scope='pred_logits')

    return logits

