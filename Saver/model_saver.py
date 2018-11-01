import tensorflow as tf


class ModelSaver(object):

    current_saver = None

    def __init__(self):
        self.current_saver = tf.train.Saver()

    def save_session(self, sess, save_name):
        self.current_saver.save(sess, save_name)
