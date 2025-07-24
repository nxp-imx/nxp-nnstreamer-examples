#!/usr/bin/env python3
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import cairo
import datetime
import gi
import logging
import math
import numpy as np
import termios
import os
import signal
import sys

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../../common/python')
sys.path.append(python_path)
from imxpy.imx_dev import Imx, SocId  # noqa
from imxpy.common_utils import GstVideoImx, store_vx_graph_compilation  # noqa

gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib


class StdInHelper:

    def __init__(self):
        """Configure stdin read blocking mode.
        """
        self.old_attributes = None
        self.background_execution = False

    def set_attr_non_blocking(self):
        """Configure stdin tty in non-blocking mode.
        Ignored if the pipeline is executed in background.
        """
        try:
            _attr = termios.tcgetattr(sys.stdin)
            attr = _attr.copy()

            attr[3] = attr[3] & ~(termios.ICANON | termios.ECHO)
            attr[3] = attr[3] & ~(termios.ICANON)
            attr[6][termios.VMIN] = 0
            attr[6][termios.VTIME] = 0
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, attr)
            self.old_attributes = _attr
            self.background_execution = False
        except termios.error as e:
            background_related_error_msg = "(25, 'Inappropriate ioctl for device')"
            assert str(e) == background_related_error_msg, (
                "termios error " + str(e) + " not related to background execution")
            self.background_execution = True

    def set_attr_restore(self):
        """Restore stdin configuration.
        Ignored if the pipeline is executed in background.
        """
        if not self.background_execution:
            termios.tcsetattr(sys.stdin, termios.TCSANOW, self.old_attributes)


class PoseExample:

    def __init__(self, video_file, video_dims, camera_device, flip=False, argv=[]):
        # video input definitions
        self.VIDEO_INPUT_WIDTH = video_dims[0]
        self.VIDEO_INPUT_HEIGHT = video_dims[1]
        # video input is cropped to be squared for model
        cropped_wh = min(self.VIDEO_INPUT_WIDTH, self.VIDEO_INPUT_HEIGHT)
        self.VIDEO_INPUT_RESIZED_WIDTH = cropped_wh
        self.VIDEO_INPUT_RESIZED_HEIGHT = cropped_wh
        self.flip = flip

        # camera definition
        self.camera_device = camera_device

        # model constants
        self.MODEL_KEYPOINT_SIZE = 17
        self.MODEL_INPUT_HEIGHT = 192
        self.MODEL_INPUT_WIDTH = 192
        self.MODEL_KEYPOINT_INDEX_Y = 0
        self.MODEL_KEYPOINT_INDEX_X = 1
        self.MODEL_KEYPOINT_INDEX_SCORE = 2
        self.MODEL_SCORE_THRESHOLD = 0.4

        # pipeline variables
        self.mainloop = None
        self.pipeline = None
        self.running = False
        self.time = datetime.datetime.now()
        self.fps = 0
        self.backend = None
        self.source = None

        self.tflite_path = None
        self.video_path = None
        self.video_file = video_file
        self.np_kpts = None
        self.tensor_transform = None
        self.tensor_filter_custom = None

        # keypoints definition
        # https://github.com/tensorflow/tfjs-models/tree/master/pose-detection#keypoint-diagram
        self.keypoints_def = [
            {'label': 'nose', 'connections': [1, 2, ]},
            {'label': 'left_eye', 'connections': [0, 3, ]},
            {'label': 'right_eye', 'connections': [0, 4, ]},
            {'label': 'left_ear', 'connections': [1, ]},
            {'label': 'right_ear', 'connections': [2, ]},
            {'label': 'left_shoulder', 'connections': [6, 7, 11, ]},
            {'label': 'right_shoulder', 'connections': [5, 8, 12, ]},
            {'label': 'left_elbow', 'connections': [5, 9, ]},
            {'label': 'right_elbow', 'connections': [6, 10, ]},
            {'label': 'left_wrist', 'connections': [7, ]},
            {'label': 'right_wrist', 'connections': [8, ]},
            {'label': 'left_hip', 'connections': [5, 12, 13, ]},
            {'label': 'right_hip', 'connections': [6, 11, 14, ]},
            {'label': 'left_knee', 'connections': [11, 15, ]},
            {'label': 'right_knee', 'connections': [12, 16, ]},
            {'label': 'left_ankle', 'connections': [13, ]},
            {'label': 'right_ankle', 'connections': [14, ]},
        ]

        assert len(self.keypoints_def) == self.MODEL_KEYPOINT_SIZE
        self.source = os.getenv('SOURCE', 'VIDEO')
        self.backend = os.getenv('BACKEND', 'CPU')
        self.gpu = os.getenv('GPU', 'GPU2D')
        self.use_gpu3d = self.gpu == 'GPU3D'
        self.imx = Imx()
        vela = self.imx.has_npu_ethos()
        store_vx_graph_compilation(self.imx)

        if self.backend == 'NPU':
            if self.imx.is_imx93() or self.imx.is_imx95():
                name = imx.name()
                raise NotImplementedError(f"Example can't run on {name} NPU")

        try:
            transform = {
                'CPU': ' tensor_transform mode=typecast option=uint8 ! ',
                'NPU': ' tensor_transform mode=typecast option=uint8 ! ',
            }
            self.tensor_transform = transform[self.backend]

            if vela:
                quantized_model = 'movenet_quant_vela.tflite'
            else:
                quantized_model = 'movenet_quant.tflite'
            model = {
                'CPU': 'movenet_quant.tflite',
                'NPU': quantized_model,
            }
            tflite_model = model[self.backend]

            if self.imx.has_npu_vsi():
                custom_ops = ('custom=Delegate:External,'
                              'ExtDelegateLib:libvx_delegate.so')
            elif vela:
                custom_ops = ('custom=Delegate:External,'
                              'ExtDelegateLib:libethosu_delegate.so')
            else:
                custom_ops = ''

            custom = {
                'NPU': custom_ops,
                'GPU': 'custom=Delegate:External,ExtDelegateLib:libvx_delegate.so',
                'CPU': f'custom=Delegate:XNNPACK,NumThreads:{os.cpu_count()//2}'
            }
            self.tensor_filter_custom = custom[self.backend]

            gpu_inference = {'NPU': '0', 'GPU': '1', 'CPU': '0', }
            os.environ['USE_GPU_INFERENCE'] = gpu_inference[self.backend]
        except Exception:
            raise ValueError('unknown backend [%s]', self.backend)

        current_folder = os.path.dirname(os.path.abspath(__file__))

        model_dir = os.path.join(
            current_folder, '../../downloads/models/pose-estimation')
        self.tflite_path = os.path.join(model_dir, tflite_model)
        if not os.path.exists(self.tflite_path):
            raise FileExistsError(
                f'cannot find tflite model [{self.tflite_path}]')

        video_dir = os.path.join(
            current_folder, '../../downloads/media/movies')
        self.video_path = os.path.join(video_dir, self.video_file)
        if not os.path.exists(self.video_path):
            raise FileExistsError(f'cannot find video [{self.video_path}]')

        signal.signal(signal.SIGINT, self.sigint_handler)

        Gst.init(argv)

    # @brief main loop execution routine
    def run(self):
        stdin = StdInHelper()
        logging.info('Press <ESC> to quit.')
        stdin.set_attr_non_blocking()

        self.mainloop = GLib.MainLoop()

        gstvideoimx = GstVideoImx(self.imx)

        # i.MX93 and i.MX95 does not support video file decoding
        if self.imx.is_imx93() and self.source == 'VIDEO':
            print('video file cannot be decoded, use camera source instead')
            self.source = 'CAMERA'

        if self.source == 'VIDEO':
            cmdline = 'filesrc location={:s} !'.format(self.video_path)
            extension = os.path.splitext(self.video_file)[1]
            print('extension', extension)
            if extension == '.webm' or extension == '.mkv':
                if self.imx.id() == SocId.IMX8MP:
                    cmdline += ' matroskademux ! v4l2vp9dec name=video-decode !'
                elif self.imx.is_imx95():
                    cmdline += ' matroskademux ! avdec_vp9 name=video-decode !'
            else:
                print('only .mkv or .webm video format can be decoded by this pipeline')
                return
            cmdline += ' video/x-raw,width={:d},height={:d} !'.format(
                self.VIDEO_INPUT_WIDTH, self.VIDEO_INPUT_HEIGHT)

        elif self.source == 'CAMERA':
            cmdline = 'v4l2src name=cam_src device={:s} num-buffers=-1 ! '.format(
                self.camera_device)
            cmdline += 'video/x-raw, width={:d},height={:d} !'.format(
                self.VIDEO_INPUT_WIDTH, self.VIDEO_INPUT_HEIGHT)
        else:
            raise ValueError('Wrong source, must be VIDEO or CAMERA')

        # crop for square video format
        crop_left = crop_right = (
            self.VIDEO_INPUT_WIDTH - self.VIDEO_INPUT_RESIZED_WIDTH) // 2
        if ((crop_left * 2 + self.VIDEO_INPUT_RESIZED_WIDTH) != self.VIDEO_INPUT_WIDTH):
            crop_right += 1
        crop_top = crop_bottom = (
            self.VIDEO_INPUT_HEIGHT - self.VIDEO_INPUT_RESIZED_HEIGHT) // 2
        if ((crop_top * 2 + self.VIDEO_INPUT_RESIZED_HEIGHT) != self.VIDEO_INPUT_HEIGHT):
            crop_bottom += 1

        cmdline += gstvideoimx.accelerated_videocrop_to_format('video_crop', top=crop_top, bottom=crop_bottom,
                                                               left=crop_left, right=crop_right, use_gpu3d=self.use_gpu3d)
        if self.flip:
            cmdline += gstvideoimx.accelerated_videoscale(
                flip=True, use_gpu3d=self.use_gpu3d)

        cmdline += ' tee name=t'
        cmdline += ' t. ! queue name=thread-nn max-size-buffers=2 leaky=2 ! '
        cmdline += gstvideoimx.accelerated_videoscale(
            self.MODEL_INPUT_WIDTH, self.MODEL_INPUT_HEIGHT, 'RGB', use_gpu3d=self.use_gpu3d)
        cmdline += ' tensor_converter !'
        cmdline += self.tensor_transform
        cmdline += ' tensor_filter name=model_inference framework=tensorflow-lite model={:s} {:s} !' \
            .format(self.tflite_path, self.tensor_filter_custom)
        cmdline += ' tensor_sink name=tensor_sink'
        cmdline += ' t. ! queue name=thread-img max-size-buffers=2 !'
        cmdline += ' videoconvert ! cairooverlay name=cairooverlay !'
        cmdline += ' waylandsink'

        print(cmdline)

        self.pipeline = Gst.parse_launch(cmdline)

        # pipeline bus and message callback
        bus = self.pipeline.get_bus()
        bus.add_signal_watch()
        bus.connect('message', self.on_bus_message)

        # tensor sink signal : new-data callback
        tensor_sink = self.pipeline.get_by_name('tensor_sink')
        tensor_sink.connect('new-data', self.new_data_cb)

        # cairooverlay signal: draw callback
        tensor_res = self.pipeline.get_by_name('cairooverlay')
        tensor_res.connect('draw', self.draw_cb)

        # start pipeline
        self.pipeline.set_state(Gst.State.PLAYING)
        self.running = True

        GLib.timeout_add(100, self.timeout_function)

        # run main loop until EOS or error reported
        self.mainloop.run()

        self.running = False
        self.pipeline.set_state(Gst.State.NULL)

        bus.remove_signal_watch()

        stdin.set_attr_restore()

    # @brief callback for tensor sink signal.
    def new_data_cb(self, sink, buffer):
        if self.running:

            # movenet model card:
            #     https://storage.googleapis.com/movenet/MoveNet.SinglePose%20Model%20Card.pdf
            #
            # output tensor #0 format: [1:1:17:3] type float32
            #   17 keypoints, 3 channels on last dimension:
            #     channel #0, channel #1: (y, x) normalized to image dimension
            #     channel #2 confidence score for the keypoint

            if buffer.n_memory() != 1:
                return False

            # tensor buffer #0
            mem_kpts = buffer.peek_memory(0)
            result, info = mem_kpts.map(Gst.MapFlags.READ)

            if result:
                assert info.size == self.MODEL_KEYPOINT_SIZE * \
                    3 * np.dtype(np.float32).itemsize

                # convert buffer to [1:1:17:3] numpy array
                np_kpts = np.frombuffer(info.data, dtype=np.float32) \
                    .reshape(self.MODEL_KEYPOINT_SIZE, -1) \
                    .copy()

                # rescale normalized keypoints (x,y) per video resolution
                np_kpts[:, self.MODEL_KEYPOINT_INDEX_X] *= \
                    self.VIDEO_INPUT_RESIZED_WIDTH
                np_kpts[:, self.MODEL_KEYPOINT_INDEX_Y] *= \
                    self.VIDEO_INPUT_RESIZED_HEIGHT

                # score confidence criteria
                for np_kpt in np_kpts:
                    score = np_kpt[self.MODEL_KEYPOINT_INDEX_SCORE]
                    valid = (score >= self.MODEL_SCORE_THRESHOLD)
                    np_kpt[self.MODEL_KEYPOINT_INDEX_SCORE] = valid

                self.np_kpts = np_kpts
            mem_kpts.unmap(info)

            # coarse fps computation
            now = datetime.datetime.now()
            delta_ms = (now - self.time).total_seconds() * 1000
            self.time = now
            self.fps = 1000 / delta_ms

    # @brief callback to draw from cairooverlay.
    def draw_cb(self, overlay, context, timestamp, duration):

        if self.np_kpts is None or not self.running:
            return

        context.select_font_face('Arial',
                                 cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
        context.set_line_width(1.0)

        for i in range(0, self.MODEL_KEYPOINT_SIZE):

            # np_pkts access is GIL protected
            np_kpt = self.np_kpts[i]
            valid = np_kpt[self.MODEL_KEYPOINT_INDEX_SCORE]
            if not valid:
                continue

            keypoint_def = self.keypoints_def[i]

            x_kpt = np_kpt[self.MODEL_KEYPOINT_INDEX_X]
            y_kpt = np_kpt[self.MODEL_KEYPOINT_INDEX_Y]

            # draw keypoint spot
            context.set_source_rgb(1, 0, 0)
            context.arc(x_kpt, y_kpt, 1, 0, 2 * math.pi)
            context.fill()
            context.stroke()

            # draw keypoint label
            context.set_source_rgb(0, 1, 1)
            context.set_font_size(10.0)
            context.move_to(x_kpt + 5, y_kpt + 5)
            label = keypoint_def['label']
            # invert left and right if video is flipped
            if self.flip:
                if "left" in label:
                    label = label.replace("left", "right")
                elif "right" in label:
                    label = label.replace("right", "left")
            context.show_text(label)

            # draw keypoint connections
            context.set_source_rgb(0, 1, 0)
            connections = keypoint_def['connections']
            for connect in connections:
                np_connect = self.np_kpts[connect]
                valid = np_connect[self.MODEL_KEYPOINT_INDEX_SCORE]
                if not valid:
                    continue

                x_connect = np_connect[self.MODEL_KEYPOINT_INDEX_X]
                y_connect = np_connect[self.MODEL_KEYPOINT_INDEX_Y]

                context.move_to(x_kpt, y_kpt)
                context.line_to(x_connect, y_connect)
            context.stroke()

            # display fps indication
            context.set_source_rgb(0.85, 0, 1)
            context.move_to(14, 14)
            context.select_font_face('Arial',
                                     cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
            context.set_font_size(11.0)
            context.show_text(f'FPS ({self.backend}): {self.fps:.1f}')

    # @brief pipeline bus message callback.
    def on_bus_message(self, bus, message):
        type = message.type
        if type == Gst.MessageType.EOS:
            logging.info('bus eos')
            self.mainloop.quit()
        elif type == Gst.MessageType.ERROR:
            err, debug = message.parse_error()
            logging.warning('bus error %s  %s', err.message, debug)
            self.mainloop.quit()
        elif type == Gst.MessageType.WARNING:
            err, debug = message.parse_warning()
            logging.warning('bus warning %s %s', err.message, debug)
        elif type == Gst.MessageType.STREAM_START:
            logging.info('bus start')

    def sigint_handler(self, signal, frame):
        print("handling interrupt.")
        self.pipeline.set_state(Gst.State.NULL)
        self.mainloop.quit()

    def timeout_function(self):
        """GLib timer callback function implementation.
        """
        restart = True
        c = sys.stdin.read(1)
        if len(c) > 0:
            if c == '\x1b':  # escape
                self.mainloop.quit()
                restart = False

        return restart


if __name__ == '__main__':
    default_video = 'Conditioning_Drill_1-_Power_Jump.webm.480p.vp9.webm'
    default_dims = (854, 480)

    imx = Imx()
    if imx.id() == SocId.IMX8MP:
        default_camera = '/dev/video3'
    elif imx.is_imx93():
        default_camera = '/dev/video0'
    elif imx.is_imx95():
        default_camera = '/dev/video13'
    else:
        name = imx.name()
        raise NotImplementedError(f'Platform not supported [{name}]')

    parser = argparse.ArgumentParser(description='Pose Detection')
    parser.add_argument('--video_file', '-f',
                        type=str,
                        help='input video file',
                        default=default_video)
    parser.add_argument('--mirror', '-m',
                        default=False, action='store_true',
                        help='flip image to display as a mirror')
    parser.add_argument('--video_dims', '-d',
                        metavar=('WIDTH', 'HEIGHT'),
                        nargs=2,
                        type=int,
                        help='input resolution (width x height)',
                        default=default_dims)
    parser.add_argument('--camera_device', '-c',
                        type=str,
                        help='camera device node',
                        default=default_camera)
    args = parser.parse_args()

    video_file = args.video_file
    camera_device = args.camera_device
    video_dims = tuple(args.video_dims)
    flip = args.mirror
    example = PoseExample(video_file, video_dims,
                          camera_device, flip, sys.argv[2:])
    example.run()
