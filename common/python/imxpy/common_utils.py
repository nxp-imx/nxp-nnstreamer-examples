#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

import os
import sys

from . import imx_dev


class GstVideoImx:
    """Helper class for video pipeline segments handling.

    imx: imxdev.Imx() instance
    """
    def __init__(self, imx):
        assert (isinstance(imx, imx_dev.Imx))
        self.imx = imx

    def videoscale_to_format(self, format=None, width=None, height=None, hardware=None):
        """Create pipeline segment for accelerated video formatting and csc.

        hardware: corresponding hardware to accelerate video composition
        format: GStreamer video format
        width: output video width after rescale
        height: output video height after rescale
        return: GStreamer pipeline segment string
        """
        valid_dimensions = width is not None and height is not None
        required_format = format is not None

        if hardware is not None:
            cmd = f'imxvideoconvert_{hardware} ! '
        else:
            # no acceleration
            if valid_dimensions:
                cmd = 'videoscale ! '
            if required_format:
                cmd += 'videoconvert ! '

        cmd += f'video/x-raw'
        if valid_dimensions:
            cmd += f',width={width},height={height}'
        if required_format:
            cmd += f',format={format}'
        cmd += ' ! '

        return cmd

    def accelerated_videoscale(self, width=None, height=None, format=None):
        """Create pipeline segment for accelerated video scaling and conversion
        to a given GStreamer video format.

        width: output video width after rescale
        height: output video height after rescale
        format: GStreamer video format
        return: GStreamer pipeline segment string
        """
        valid_dim_g2d = valid_dim_pxp = True
        valid_dimensions = width is not None and height is not None
        if valid_dimensions:
            # g2d and pxp can't resize under 64 pixels
            valid_dim_g2d = valid_dim_pxp = width >= 64 and height >= 64

        if self.imx.has_g2d() and valid_dim_g2d:
            # imxvideoconvert_g2d does not support RGB nor GRAY8 sink
            # use acceleration to RGBA
            if format == 'RGB' or format == 'GRAY8':
                cmd = self.videoscale_to_format('RGBA', width, height, 'g2d')
                cmd += f'videoconvert ! video/x-raw,format={format} ! '
            else:
                cmd = self.videoscale_to_format(format, width, height, 'g2d')
        elif self.imx.has_pxp() and valid_dim_pxp:
            if format == 'RGB':
                # imxvideoconvert_pxp does not support RGB sink
                # use acceleration to BGR
                cmd = self.videoscale_to_format('BGR', width, height, 'pxp')
                cmd += f'videoconvert ! video/x-raw,format={format} ! '
            else:
                cmd = self.videoscale_to_format(format, width, height, 'pxp')

        else:
            # no hardware acceleration
            cmd = self.videoscale_to_format(format, width, height)
        return cmd

    def videocrop_to_format(self, videocrop_name, width, height, top=None, bottom=None,
                            left=None, right=None, format=None):
        """Create pipeline segment for accelerated video cropping and conversion
        to a given GStreamer video format.

        videocrop_name: GStreamer videocrop element name
        width: output video width after rescale
        height: output video height after rescale
        top: top pixels to be cropped - may be setup via property later
        bottom: bottom pixels to be cropped - may be setup via property later
        left: left pixels to be cropped - may be setup via property later
        right: right pixels to be cropped - may be setup via property later
        format: GStreamer video format
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

        cmd += self.accelerated_videoscale(width, height, format)

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
