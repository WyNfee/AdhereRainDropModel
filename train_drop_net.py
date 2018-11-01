import os
import tensorflow as tf
from Optimizer import optimizer_manager
from Deploy import deployment as dpm
from Saver import model_saver
from Restorer import model_restorer
from Model import drop_net
from Data import drop_data_reader as reader

DATA_TFRECORD_FILE = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Train\record\drop.tfrecords"

TRAIN_BATCH_SIZE = 8
TRAIN_INIT_LEARNING_RATE = 1e-3
TRAIN_LEARNING_RATE_DECAY_STEP = 2500
# final learning rate = TRAIN_LEARNING_RATE * pow(0.95, TRAIN_ITERATE_STEPS / TRAIN_LEARNING_RATE_DECAY_STEP)
TRAIN_LEARNING_RATE_DECAY_FACTOR = 0.95
TRAIN_ITERATE_STEPS = 200001  # None means endless training

RESTORE_CHECKPOINT_FILE = None
# None means everything will be restored
RESTORE_INCLUDE_SCOPES = None

RESTORE_EXCLUDE_SCOPES = None  # None means nothing will be excluded when restore
# None means everything is trainable
RESTORE_TRAINABLE_SCOPES = None

OPTIMIZER_NAME = 'adam'  # adam, momentum supported
OPTIMIZER_PARAMETERS = [0.9, 0.999, 1e-8]


EVENTS_DIR = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Train\save"
LOG_SAVE_DIR = r"C:\Users\wangy\Documents\GitHub\AdhereRainDropModel\_Train\save"
LOG_SAVE_FILE_PATTERN = "DROP_DATA"


def main(_):
    deployment = dpm.Deployment()

    with tf.device(deployment.get_input_device()):
        global_step = tf.train.create_global_step()

        images, labels = reader.create_input(DATA_TFRECORD_FILE, TRAIN_BATCH_SIZE)

    # define model
    with tf.device(deployment.get_model_device()):
        logits = drop_net.get_net(images)

        # LOSS COMPUTATION and NEGATIVE MINING
        positive_mask = tf.greater(labels, 0)
        positive_sample_amount = tf.reduce_sum(tf.cast(positive_mask, dtype=tf.float32))
        tf.summary.scalar('PositiveSampleAmount', positive_sample_amount)

        negative_mask = tf.logical_not(positive_mask)
        negative_sample_amount = tf.reduce_sum(tf.cast(negative_mask, dtype=tf.int32))
        negative_sample_planned_amount = tf.cast(3. * positive_sample_amount, dtype=tf.int32)
        negative_sample_amount_if_no_positive = tf.div(negative_sample_amount, int(8))
        final_negative_sample_amount = tf.minimum(negative_sample_amount, negative_sample_planned_amount)
        final_negative_sample_amount = tf.maximum(final_negative_sample_amount, negative_sample_amount_if_no_positive)

        no_negative_sample_mask = tf.equal(final_negative_sample_amount, 0)
        final_negative_sample_amount = tf.cond(no_negative_sample_mask, lambda: tf.cast(3., dtype=tf.int32), lambda: final_negative_sample_amount)
        final_negative_sample_amount = tf.minimum(final_negative_sample_amount, negative_sample_amount)
        tf.summary.scalar('NegativeSampleAmount', final_negative_sample_amount)

        negative_confidence = tf.nn.softmax(logits)
        negative_sample_cast = tf.cast(negative_mask, tf.float32)

        # if a value is for negative sample, then give them confidence as predict
        # if a value is for positive sample, then set it to 1
        negative_confidence_for_filter = tf.where(negative_mask, negative_confidence[:, :, :, 0], (1 - negative_sample_cast))
        # flatten the filter values,
        negative_confidence_for_filter_flatten = tf.reshape(negative_confidence_for_filter, [-1])
        # finding the top-k value,
        # notice we use negative sign for negative confidence,
        # this means those which it not believed as negative, e.g. predict as (0.1) , will be comes the largest (-0.1).
        # those fully confidence to negative (0.9) or not negative at all (set as 1) will be smallest (-0.9) or (-1)
        top_k_value_for_negative_mining, _ = tf.nn.top_k(-negative_confidence_for_filter_flatten, k=final_negative_sample_amount)
        # picking the smallest one for top_k item,
        # notice, it is a negative value (such like -0.3)
        top_k_count = tf.shape(top_k_value_for_negative_mining)[0]
        top_k_has_item_cond = top_k_count > 0

        value_for_negative_mining = tf.cond(top_k_has_item_cond,
                                            lambda: top_k_value_for_negative_mining[-1],
                                            lambda: tf.negative(tf.ones(())))

        final_negative_mask = tf.logical_and(negative_mask, -negative_confidence_for_filter >= value_for_negative_mining)
        final_negative_cast = tf.cast(final_negative_mask, dtype=tf.float32)
        final_positive_cast = tf.cast(positive_mask, dtype=tf.float32)

        error_pos = tf.nn.sparse_softmax_cross_entropy_with_logits(logits=logits, labels=labels)
        error_pos = tf.losses.compute_weighted_loss(error_pos, final_positive_cast)

        error_neg = tf.nn.sparse_softmax_cross_entropy_with_logits(logits=logits, labels=labels)
        error_neg = tf.losses.compute_weighted_loss(error_neg, final_negative_cast)

        error = error_pos + error_neg

        learning_rate = tf.train.exponential_decay(
            TRAIN_INIT_LEARNING_RATE, global_step,
            TRAIN_LEARNING_RATE_DECAY_STEP, TRAIN_LEARNING_RATE_DECAY_FACTOR, staircase=True)

        opt_mgr = optimizer_manager.OptimizerManager(OPTIMIZER_NAME, learning_rate, OPTIMIZER_PARAMETERS)
        opt_mgr.set_trainable_variables(RESTORE_TRAINABLE_SCOPES)
        train_step = opt_mgr.minimize_error(error, global_step)

    # create summary writer
    train_writer = tf.summary.FileWriter(EVENTS_DIR)
    train_writer.add_graph(tf.get_default_graph())

    # Creating Summaries
    # add loss as scalar
    tf.summary.scalar('loss', error)
    tf.summary.scalar('lr', learning_rate)
    summaries = set(tf.get_collection(tf.GraphKeys.SUMMARIES))
    summary_op = tf.summary.merge(list(summaries))

    init_op = tf.group(tf.global_variables_initializer(),
                       tf.local_variables_initializer())

    # setting gpu usage
    config = tf.ConfigProto()
    config.gpu_options.allow_growth = True
    config.gpu_options.per_process_gpu_memory_fraction = 0.5
    config.allow_soft_placement = True

    sess = tf.Session(config=config)
    sess.run(init_op)

    saver = model_saver.ModelSaver()

    restorer = model_restorer.ModelRestorer(RESTORE_INCLUDE_SCOPES, RESTORE_EXCLUDE_SCOPES)
    restorer.restore_model(sess, RESTORE_CHECKPOINT_FILE)

    coord = tf.train.Coordinator()
    threads = tf.train.start_queue_runners(sess=sess, coord=coord)

    update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)

    try:
        step = 0
        while TRAIN_ITERATE_STEPS is None or step < TRAIN_ITERATE_STEPS:

            _, loss, _ = sess.run([train_step, error, update_ops])

            if step % 10 == 0 and step != 0:
                print('Step %d: loss %.4f' % (step, loss))

            if step % 100 == 0 and step != 0:
                summary_data = sess.run([summary_op])
                for i in range(len(summary_data)):
                    train_writer.add_summary(summary_data[i], step)
                print('Adding new summary to file for step %d' % step)

            if step % 1000 == 0 and step != 0:
                # save the model
                save_name = LOG_SAVE_FILE_PATTERN + '_%d' % step + '.ckpt'
                save_name = os.path.join(LOG_SAVE_DIR, save_name)
                saver.save_session(sess, save_name)
                print('Model Save to %s' % save_name)

            step = step + 1
    finally:
        coord.request_stop()
    coord.join(threads)

    sess.close()


if __name__ == '__main__':
    tf.app.run()
