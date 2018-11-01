import tensorflow as tf
from tensorflow.contrib.framework import *


class ModelRestorer(object):
    current_restore = None

    def __init__(self, include_scope, exclude_scope):
        variable_list = get_variables_to_restore(include=include_scope, exclude=exclude_scope)
        self.current_restore = tf.train.Saver(var_list=variable_list)

    def restore_model(self, session, restore_file):
        if restore_file is not None:
            self.current_restore.restore(sess=session, save_path=restore_file)
