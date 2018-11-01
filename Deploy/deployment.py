from tensorflow.python.client import device_lib


class Deployment(object):

    def __init__(self):
        local_devices = device_lib.list_local_devices()
        cpu_devices = [x.name for x in local_devices if x.device_type == 'CPU']
        gpu_devices = [x.name for x in local_devices if x.device_type == 'GPU']
        self._local_devices = local_devices
        self._cpu_devices = cpu_devices
        self._gpu_devices = gpu_devices

    @property
    def cpu_devices(self):
        return self._cpu_devices

    @property
    def gpu_devices(self):
        return self._gpu_devices

    def get_gpu_amount(self):
        return len(self._gpu_devices)

    def get_cpu_amount(self):
        return len(self._cpu_devices)

    def get_gpu_by_idx(self, idx):
        if idx > len(self.gpu_devices) - 1:
            raise ValueError('IDX is bigger than actual GPU amount, GPU amount is %d, idx is %d' %
                             (self.get_gpu_amount(), idx))
        return self.gpu_devices[idx]

    def get_cpu_by_idx(self, idx):
        if idx > len(self.cpu_devices) - 1:
            raise ValueError('IDX is bigger than actual CPU amount, CPU amount is %d, idx is %d' %
                             (self.get_cpu_amount(), idx))
        return self.cpu_devices[idx]

    def get_input_device(self):
        if self.get_cpu_amount() > 0:
            return self.get_cpu_by_idx(0)
        else:
            raise ValueError('cannot find any suitable device')

    def get_model_device(self, idx=0):
        if self.get_gpu_amount() > 0:
            if self.get_gpu_amount() > idx:
                return self.get_gpu_by_idx(idx)
            else:
                print('cannot find request gpu, using default')
                return self.get_gpu_by_idx(0)
        elif self.get_cpu_amount() > 0:
            return self.get_cpu_by_idx(0)
        else:
            raise ValueError('cannot find any suitable device')


