import math

import numpy as np

from onireader._onireader import Device as _Device
from onireader._onireader import ANY_DEVICE, PixelFormat


class Intrinsics:
    def __init__(self, fx, fy, cx, cy):
        self.fx = fx
        self.fy = fy
        self.cx = cx
        self.cy = cy

    def __str__(self):
        return "Intrinsics: fx={}, fy={}, cx={}, cy={}".format(
            self.fx, self.fy, self.cx, self.cy)

    def __repr__(self):
        return str(self)


class Device(_Device):
    def __init__(self):
        super(Device, self).__init__()

    def open(self, device_uri=None):
        if device_uri is None:
            device_uri = ""
        return super(Device, self).open(device_uri)

    def find_best_fit_modes(self, width, height,
                            depth_format=PixelFormat.DEPTH_1_MM,
                            rgb_format=PixelFormat.RGB888):
        depth_vmodes = self.get_depth_video_modes()
        rgb_vmodes = self.get_color_video_modes()

        target_res = np.array([width, height])

        depth_res_dist = [
            (mode_num,
             np.linalg.norm(target_res - np.array([vmode.width, vmode.height])))
            for mode_num, vmode in enumerate(depth_vmodes)
            if vmode.format == depth_format]

        rgb_res_dist = [
            (mode_num,
             np.linalg.norm(target_res - np.array([vmode.width, vmode.height])))
            for mode_num, vmode in enumerate(rgb_vmodes)
            if vmode.format == rgb_format]

        depth_mode = sorted(depth_res_dist, key=lambda x: x[1])[0][0]
        rgb_mode = sorted(rgb_res_dist, key=lambda x: x[1])[0][0]

        return depth_mode, rgb_mode

    def start(self, depth_mode=-1, rgb_mode=-1):
        super(Device, self).start(depth_mode, rgb_mode)

    def get_intrinsics(self):
        hfov = self.get_horizontal_fov()
        vfov = self.get_vertical_fov()

        vmode = self.get_depth_video_mode()
        import ipdb
        ipdb.set_trace()

        fx = vmode.width * .5 / math.tan(hfov)
        fy = vmode.height * .5 / math.tan(vfov)
        cx = vmode.width * .5
        cy = vmode.height * .5

        return Intrinsics(fx, fy, cx, cy)
