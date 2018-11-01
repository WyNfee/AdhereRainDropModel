# this script provide optimizer of adam, with default value
import tensorflow as tf


def get_optimizer(learning_rate, parameters):
    optimizer = tf.train.AdamOptimizer(learning_rate, beta1=parameters[0], beta2=parameters[1], epsilon=parameters[2])
    return optimizer
