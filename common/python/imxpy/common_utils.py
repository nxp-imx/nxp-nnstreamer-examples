#!/usr/bin/env python3
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import os

from . import imx_dev


class GstVideoImx:
    """Helper class for video pipeline segments handling.

    imx: imxdev.Imx() instance
    """

    def __init__(self, imx):
        assert (isinstance(imx, imx_dev.Imx))
        self.imx = imx
        # counter used to avoid duplicated names in the pipeline
        self.cnt_element_names = 0

    def video_flip(self, hardware=None, flip_only=None, keep_image_ratio=False):
        """Create pipeline segment for accelerated video flip.

        hardware: corresponding hardware to accelerate video composition
        flip_only: if no other operations than image flip are required
        keep_image_ratio: resize while maintaining image aspect ratio
        return: GStreamer pipeline segment string
        """
        if hardware is not None:
            if flip_only:
                operation_name = f"flip_{hardware}_{self.cnt_element_names}"
            else:
                operation_name = f"scale_csc_flip_{hardware}_{self.cnt_element_names}"
            cmd = f'imxvideoconvert_{hardware} name={operation_name} '
            cmd += 'rotation=4 '
            if not flip_only and keep_image_ratio:
                cmd += 'keep-ratio=True '
            cmd += '! '
        else:
            # no acceleration
            cmd = f'videoflip video-direction=4 name=flip_cpu_{self.cnt_element_names} ! '
        self.cnt_element_names += 1

        return cmd

    def videoscale_to_format(self, format=None, width=None, height=None, hardware=None, flip=False, cropping=False, keep_image_ratio=False):
        """Create pipeline segment for accelerated video formatting and csc.

        format: GStreamer video format
        width: output video width after rescale
        height: output video height after rescale
        hardware: corresponding hardware to accelerate video composition
        flip: mirror mode, flip the image (not available on 3D GPU)
        cropping: enable video cropping acceleration
        keep_image_ratio: resize while maintaining image aspect ratio
        return: GStreamer pipeline segment string
        """
        valid_dimensions = width is not None and height is not None
        required_format = format is not None
        cmd = ''

        # In case function is called only for flipping
        flip_only = valid_dimensions is False and required_format is False

        if hardware is not None:
            if flip:
                cmd = self.video_flip(hardware=hardware, flip_only=flip_only)
            else:
                if cropping:
                    cmd = f'imxvideoconvert_{hardware} name=video_crop_scale_csc_{hardware}_{self.cnt_element_names} videocrop-meta-enable=true ! '
                else:
                    cmd = f'imxvideoconvert_{hardware} name=scale_csc_{hardware}_{self.cnt_element_names} '
                    if keep_image_ratio:
                        cmd += 'keep-ratio=True '
                    cmd += ' ! '
                self.cnt_element_names += 1
        else:
            # no acceleration
            if valid_dimensions:
                cmd += f'videoscale name=scale_cpu_{self.cnt_element_names} ! '
                self.cnt_element_names += 1
            if flip:
                cmd += self.video_flip(hardware=None, flip_only=flip_only)
            if required_format:
                cmd += f'videoconvert name=csc_cpu_{self.cnt_element_names} ! '
                self.cnt_element_names += 1

        # Add a caps if any change in resolution or format is expected
        if valid_dimensions or required_format:
            cmd += f'video/x-raw'
            if valid_dimensions:
                cmd += f',width={width},height={height}'
            if required_format:
                cmd += f',format={format}'
            cmd += ' ! '

        return cmd

    def accelerated_videoscale(self, width=None, height=None, format=None, flip=False, use_gpu3d=False, cropping=False, keep_image_ratio=False):
        """Create pipeline segment for accelerated video scaling and conversion
        to a given GStreamer video format.

        width: output video width after rescale
        height: output video height after rescale
        format: GStreamer video format
        flip: mirror mode, flip the image (not available on 3D GPU)
        use_gpu3d: use the GPU3D instead of GPU2D if available
        cropping: enable video cropping acceleration
        keep_image_ratio: resize while maintaining image aspect ratio
        return: GStreamer pipeline segment string
        """

        valid_dimensions = width is not None and height is not None
        required_format = format is not None
        cmd = ''

        if use_gpu3d:
            if self.imx.has_gpu3d():
                # flip is not supported on 3D GPU, do flip first on other HW without resizing
                if flip:
                    cmd = self.accelerated_videoscale(
                        flip=True, use_gpu3d=False)
                # imxvideoconvert_ocl does not support GRAY8 sink
                # use acceleration to YUY2
                if valid_dimensions or required_format:
                    if format == 'GRAY8':
                        cmd += self.videoscale_to_format(
                            'YUY2', width, height, 'ocl', False, cropping, keep_image_ratio)
                        cmd += f'videoconvert name=gray_convert_cpu_{self.cnt_element_names} ! video/x-raw,format={format} ! '
                        self.cnt_element_names += 1
                    else:
                        cmd += self.videoscale_to_format(
                            format, width, height, 'ocl', False, cropping, keep_image_ratio)
            else:
                # no 3D GPU acceleration
                print(
                    'this target has no GPU3D support, operations will be executed on supported HW instead')
                cmd = self.accelerated_videoscale(
                    width, height, format, flip, False, cropping, keep_image_ratio)
        else:  # Use GPU2D or CPU
            if self.imx.has_g2d():
                # imxvideoconvert_g2d does not support RGB nor GRAY8 sink on i.MX8 platforms
                # use acceleration to RGBA instead
                if self.imx.is_imx8() and (format == 'RGB' or format == 'GRAY8'):
                    cmd = self.videoscale_to_format(
                        'RGBA', width, height, 'g2d', flip, cropping, keep_image_ratio)
                    format_name = 'rgb' if format == 'RGB' else 'gray'
                    cmd += f'videoconvert name={format_name}_convert_cpu_{self.cnt_element_names} ! video/x-raw,format={format} ! '
                    self.cnt_element_names += 1
                 # imxvideoconvert_g2d does not support GRAY8 sink on i.MX95 platform
                 # use acceleration to YUY2 instead
                 #TODO: to remove condition when GRAY8 will be supported
                elif self.imx.is_imx95() and format == 'GRAY8':
                    cmd = self.videoscale_to_format(
                        'YUY2', width, height, 'g2d', flip, cropping, keep_image_ratio)
                    cmd += f'videoconvert name=gray_convert_cpu_{self.cnt_element_names} ! video/x-raw,format={format} ! '
                else:
                    cmd = self.videoscale_to_format(
                        format, width, height, 'g2d', flip, cropping, keep_image_ratio)
            elif self.imx.has_pxp():
                if format == 'RGB':
                    # imxvideoconvert_pxp does not support RGB sink
                    # use acceleration to BGR
                    cmd = self.videoscale_to_format(
                        'BGR', width, height, 'pxp', flip, keep_image_ratio=keep_image_ratio)
                    cmd += f'videoconvert name=rgb_convert_cpu_{self.cnt_element_names} ! video/x-raw,format={format} ! '
                    self.cnt_element_names += 1
                else:
                    cmd = self.videoscale_to_format(
                        format, width, height, 'pxp', flip, keep_image_ratio=keep_image_ratio)
            else:
                # no hardware acceleration
                cmd = self.videoscale_to_format(
                    format, width, height, flip=flip, keep_image_ratio=keep_image_ratio)

        return cmd

    def accelerated_videocrop_to_format(self, videocrop_name, width=None, height=None, top=None, bottom=None,
                                        left=None, right=None, format=None, flip=False, use_gpu3d=False, keep_image_ratio=False):
        """Create pipeline segment for accelerated video cropping and conversion
        to a given GStreamer video format.

        videocrop_name: GStreamer videocrop element name
        width: output video width after rescale (done after cropping)
        height: output video height after rescale (done after cropping)
        top: top pixels to be cropped - may be setup via property later
        bottom: bottom pixels to be cropped - may be setup via property later
        left: left pixels to be cropped - may be setup via property later
        right: right pixels to be cropped - may be setup via property later
        format: GStreamer video format
        flip: mirror mode, flip the image (not available on 3D GPU)
        use_gpu3d: use the GPU3D instead of GPU2D if available
        keep_image_ratio: resize while maintaining image aspect ratio
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

        cmd += self.accelerated_videoscale(width, height,
                                           format, flip, cropping=True, use_gpu3d=use_gpu3d, keep_image_ratio=keep_image_ratio)

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


def store_vx_graph_compilation(imx):
    """Store on disk .nb files that contains the result of the OpenVX graph compilation
    This feature is only available for iMX8 platforms to get the warmup time only once

    imx: imxdev.Imx() instance
    """
    is_vsi_platform = imx.has_npu_vsi() or imx.has_gpu_vsi()
    if is_vsi_platform:
        # Set the environment variables to store graph compilation on home directory
        os.environ['VIV_VX_ENABLE_CACHE_GRAPH_BINARY'] = '1'
        HOME = os.getenv('HOME')
        os.environ['VIV_VX_CACHE_BINARY_GRAPH_DIR'] = HOME

def disable_zero_copy_neutron(imx):
    """ Disable input tensor zero-copy feature not yet supported by NNStreamer
    This feature was enabled by default for Neutron NPUs

    imx: imxdev.Imx() instance
    """
    if imx.has_npu_neutron():
        os.environ['NEUTRON_ENABLE_ZERO_COPY'] = '0'
