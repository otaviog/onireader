from onireader._onireader import Device as _Device
from onireader._onireader import ANY_DEVICE


class Device(_Device):
    def __init__(self):
        super(Device, self).__init__()

    def open(self, device_uri=None):
        if device_uri is None:
            device_uri = ""
        return super(Device, self).open(device_uri)

    def start(self, depth_mode=-1, rgb_mode=-1):
        super(Device, self).start(depth_mode, rgb_mode)
