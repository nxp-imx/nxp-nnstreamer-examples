#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022 NXP

import os
import sys

from . import imx_dev


class GstVideoImx:
    def __init__(self, imx):
        """Helper class for video pipeline segments handling.

        imx: imxdev.Imx() instance
        """
        assert (isinstance(imx, imx_dev.Imx))
        self.imx = imx

    def videoscale_to_format(self, format, width=None, height=None):
        """Create pipeline segment for accelerated video formatting and csc.

        format: GStreamer video format
        width: output video width after rescale
        height: output video height after rescale
        return: GStreamer pipeline segment string
        """
        if self.imx.has_g2d():
            cmd = 'imxvideoconvert_g2d ! '
        elif self.imx.has_pxp():
            cmd = 'imxvideoconvert_pxp ! '
        else:
            # no acceleration
            cmd = 'videoscale ! videoconvert ! '

        if width is not None and height is not None:
            cmd += f'video/x-raw,width={width},'
            cmd += f'height={height},format={format} ! '
        else:
            cmd += f'video/x-raw,format={format} ! '

        return cmd

    def videoscale_to_rgb(self, width, height):
        """Create pipeline segment for accelerated video scaling and
        conversion to RGB format.

        width: output video width after rescale
        height: output video height after rescale
        return: GStreamer pipeline segment string
        """
        if self.imx.has_g2d():
            # imxvideoconvert_g2d does not support RGB sink
            # use acceleration to RGBA
            cmd = self.videoscale_to_format('RGBA', width, height)
            cmd += f'videoconvert ! video/x-raw,format=RGB ! '
        elif self.imx.has_pxp():
            # imxvideoconvert_pxp does not support RGB sink
            # use acceleration to BGR
            cmd = self.videoscale_to_format('BGR', width, height)
            cmd += f'videoconvert ! video/x-raw,format=RGB ! '
        else:
            # no acceleration
            cmd = self.videoscale_to_format('RGB', width, height)
        return cmd

    def videocrop_to_rgb(self, videocrop_name, width, height,
                         top=None, bottom=None, left=None, right=None):
        """Create pipeline segment for accelerated video cropping and
        conversion to RGB format.

        videocrop_name: GStreamer videocrop element name
        width: output video width after rescale
        height: output video height after rescale
        top: top pixels to be cropped - may be setup via property later
        bottom: bottom pixels to be cropped - may be setup via property later
        left: left pixels to be cropped - may be setup via property later
        right: right pixels to be cropped - may be setup via property later
        return: GStreamer pipeline segment string
        """
        cmd = f'  videocrop name={videocrop_name} '
        if top is not None:
            cmd += f'top={top} '
        if bottom is not None:
            cmd += f'bottom={bottom} '
        if left is not None:
            cmd += f'left={left} '
        if right is not None:
            cmd += f'right={right} '
        cmd += '! '

        cmd += self.videoscale_to_rgb(width, height)

        return cmd

    def video_compositor(self, latency=0):
        """
        Create pipeline segment for accelerated video mixing
        """
        name = "mix"
        nnst_stream = 'sink_0'
        main_stream = 'sink_1'
        options = {
            nnst_stream: {'zorder': 2},
            main_stream: {'zorder': 1}
        }

        if self.imx.has_g2d():
            cmd = f'imxcompositor_g2d name={name} '
        elif self.imx.has_pxp():
            # imxcompositor_pxp does not support RGBA sink
            # use acceleration to BGR
            cmd = f'imxcompositor_pxp name={name} '
            alpha = {'alpha': 0.3}
            options[nnst_stream].update(alpha)
        else:
            # no acceleration
            cmd = f'compositor name={name} '

        for sink in options:
            for k, v in options[sink].items():
                cmd += f'{sink}::{k}={v} '

        if latency != 0:
            cmd += f'latency={latency} min-upstream-latency={latency} '
        cmd += '! '
        return cmd