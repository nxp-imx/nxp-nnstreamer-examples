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
from imxpy.common_utils import get_default_camera_device, get_camera_backend  # noqa

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../common')
sys.path.append(python_path)
from facedetectpipe import FaceDetectPipe  # noqa

if __name__ == '__main__':

    imx = Imx()
    default_camera = get_default_camera_device(imx)

    # Set default resolution and FPS based on camera backend
    camera_backend = get_camera_backend(imx)
    if camera_backend == "libcamera":
        default_dims = (1920, 1080)
        default_fps = 60
    else:
        default_dims = (640, 480)
        default_fps = 30

    parser = argparse.ArgumentParser(description='Face Identification')
    parser.add_argument('--camera_device', '-c', type=str,
                        help='camera device node', default=default_camera)
    parser.add_argument('--mirror', '-m',
                        default=False, action='store_true',
                        help='flip image to display as a mirror')
    parser.add_argument('--video_dims', '-d',
                        metavar=('WIDTH', 'HEIGHT'),
                        nargs=2,
                        type=int,
                        help='input resolution (width x height)',
                        default=default_dims)
    parser.add_argument('--fps', '-f',
                        type=int,
                        help='camera framerate',
                        default=default_fps)
    args = parser.parse_args()

    format = '%(asctime)s.%(msecs)03d %(levelname)s:\t%(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'
    logging.basicConfig(level=logging.INFO, format=format, datefmt=datefmt)

    # pipeline parameters - no secondary pipeline
    camera_device = args.camera_device
    flip = args.mirror
    vr = tuple(args.video_dims)
    fps = args.fps
    secondary = None

    pipe = FaceDetectPipe(camera_device=camera_device, video_resolution=vr,
                          video_fps=fps, flip=flip, secondary_pipe=secondary)
    pipe.run()
