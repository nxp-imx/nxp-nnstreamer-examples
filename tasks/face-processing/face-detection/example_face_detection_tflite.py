#!/usr/bin/env python3
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import logging
import os
import sys

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../../../common/python')
sys.path.append(python_path)
from imxpy.imx_dev import Imx, SocId  # noqa
from imxpy.common_utils import get_default_camera_device  # noqa

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../common')
sys.path.append(python_path)
from facedetectpipe import FaceDetectPipe  # noqa

if __name__ == '__main__':

    imx = Imx()
    default_camera = get_default_camera_device(imx)

    parser = argparse.ArgumentParser(description='Face Identification')
    parser.add_argument('--camera_device', '-c', type=str,
                        help='camera device node', default=default_camera)
    parser.add_argument('--mirror', '-m',
                        default=False, action='store_true',
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
