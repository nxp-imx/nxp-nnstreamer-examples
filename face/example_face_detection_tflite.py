#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

import argparse
from facedetectpipe import FaceDetectPipe
import logging
import os
import sys

python_path = os.path.join(
    os.path.dirname(
        os.path.abspath(__file__)),
    '../common/python')
from imxpy.imx_dev import Imx, SocId  # noqa

if __name__ == '__main__':

    imx = Imx()
    soc = imx.id()

    if soc == SocId.IMX8MP:
        default_camera = '/dev/video3'
    elif soc == SocId.IMX93:
        default_camera = '/dev/video0'
    else:
        name = imx.name()
        raise NotImplementedError(f'Platform not supported [{name}]')

    parser = argparse.ArgumentParser(description='Face Identification')
    parser.add_argument('--camera_device', type=str,
                        help='camera device node', default=default_camera)
    parser.add_argument('--mirror',  default=False, action='store_true',
                        help='flip image to display as a mirror')
    args = parser.parse_args()

    format = '%(asctime)s.%(msecs)03d %(levelname)s:\t%(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'
    logging.basicConfig(level=logging.INFO, format=format, datefmt=datefmt)

    # pipeline parameters - no secondary pipeline
    camera_device = args.camera_device
    flip = args.mirror
    vr = (640, 480)
    fps = 30
    secondary = None

    pipe = FaceDetectPipe(camera_device=camera_device, video_resolution=vr,
                          video_fps=fps, flip=flip, secondary_pipe=secondary)
    pipe.run()
