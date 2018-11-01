import tensorflow as tf
import Optimizer.opt_adam as adam
import Optimizer.opt_momentum as momentum


class OptimizerManager(object):

    current_optimizer = None

    trainable_variables = None

    def __init__(self, optimizer_name, learning_rate, parameters):
        if optimizer_name == 'adam':
            self.current_optimizer = adam.get_optimizer(learning_rate, parameters)
        elif optimizer_name == 'momentum':
            self.current_optimizer = momentum.get_optimizer(learning_rate, parameters)
        else:
            raise ValueError("No Such Optimizer")

    def set_trainable_variables(self, trainable_list):

        if trainable_list is not None:
            trainable_variables = tf.trainable_variables()
            for trainable in trainable_variables:
                found = False
                for scope in trainable_list:
                    if trainable.name.startswith(scope):
                        found = True
                        break
                if found is True:
                    self.trainable_variables.append(trainable)
        else:
            self.trainable_variables = None

    def minimize_error(self, target, global_step):
        return self.current_optimizer.minimize(target, global_step, var_list=self.trainable_variables)


