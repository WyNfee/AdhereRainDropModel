import tensorflow as tf


def get_optimizer(learning_rate, parameters):
    optimizer = tf.train.MomentumOptimizer(learning_rate, momentum=parameters[0])
    return optimizer
