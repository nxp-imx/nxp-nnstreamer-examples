#!/usr/bin/env python3
# SPDX-License-Identifier:BSD-3-Clause
# Copyright 2022 NXP

import ctypes
import cairo
import datetime
import gi
import logging
import math
import numpy as np
import os
import sys

gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

class PoseExample:

    def __init__(self, video_file, video_dims, argv = []):
        # video input definitions
        self.VIDEO_INPUT_WIDTH = video_dims[0]
        self.VIDEO_INPUT_HEIGHT = video_dims[1]
        # video input is cropped to be squared for model
        cropped_wh = min(self.VIDEO_INPUT_WIDTH, self.VIDEO_INPUT_HEIGHT)
        self.VIDEO_INPUT_CROPPED_WIDTH = cropped_wh
        self.VIDEO_INPUT_CROPPED_HEIGHT = cropped_wh

        # model constants
        self.MODEL_KEYPOINT_SIZE = 17
        self.MODEL_INPUT_HEIGHT = 192
        self.MODEL_INPUT_WIDTH = 192
        self.MODEL_KEYPOINT_INDEX_Y = 0
        self.MODEL_KEYPOINT_INDEX_X = 1
        self.MODEL_KEYPOINT_INDEX_SCORE = 2
        self.MODEL_SCORE_THRESHOLD = 0.5

        # pipeline variables
        self.mainloop = None
        self.pipeline = None
        self.running = False
        self.time = datetime.datetime.now()
        self.fps = 0
        self.backend = None

        self.tflite_path = None
        self.video_path = None
        self.video_file = video_file
        self.np_kpts = None
        self.tensor_transform = None
        self.tensor_filter_custom = None

        # keypoints definition
        # https://github.com/tensorflow/tfjs-models/tree/master/pose-detection#keypoint-diagram
        self.keypoints_def = [
            {'label': 'nose',               'connections': [1, 2,]},
            {'label': 'left_eye',           'connections': [0, 3,]},
            {'label': 'right_eye',          'connections': [0, 4,]},
            {'label': 'left_ear',           'connections': [1,]},
            {'label': 'right_ear',          'connections': [2,]},
            {'label': 'left_shoulder',      'connections': [6, 7, 11,]},
            {'label': 'right_shoulder',     'connections': [5, 8, 12,]},
            {'label': 'left_elbow',         'connections': [5, 9,]},
            {'label': 'right_elbow',        'connections': [6, 10,]},
            {'label': 'left_wrist',         'connections': [7,]},
            {'label': 'right_wrist',        'connections': [8,]},
            {'label': 'left_hip',           'connections': [5, 12, 13,]},
            {'label': 'right_hip',          'connections': [6, 11, 14,]},
            {'label': 'left_knee',          'connections': [11, 15,]},
            {'label': 'right_knee',         'connections': [12, 16,]},
            {'label': 'left_ankle',         'connections': [13,]},
            {'label': 'right_ankle',        'connections': [14,]},
            ]

        assert len(self.keypoints_def) == self.MODEL_KEYPOINT_SIZE

        # XXX: only CPU backend for now
        self.backend = os.getenv('BACKEND', 'CPU')
        if self.backend != 'CPU':
            raise ValueError('CPU backend only is supported')

        # TODO: backend-specific configurations
        try:
            transform = {
                'CPU': ' tensor_transform mode=typecast option=int32 ! ',
                }
            self.tensor_transform = transform[self.backend]

            model = {
                'CPU':'movenet_single_pose_lightning.tflite',
                }
            tflite_model = model[self.backend]

            custom = {
                'NPU':'custom=Delegate:External,ExtDelegateLib:libvx_delegate.so',
                'GPU':'custom=Delegate:External,ExtDelegateLib:libvx_delegate.so',
                'CPU':f'custom=Delegate:XNNPACK,NumThreads:{os.cpu_count()}'
                }
            self.tensor_filter_custom = custom[self.backend]

            gpu_inference = {'NPU': '0', 'GPU': '1', 'CPU': '0',}
            os.environ['USE_GPU_INFERENCE'] = gpu_inference[self.backend]
        except Exception:
           raise ValueError('unknown backend [%s]', self.backend)

        current_folder = os.path.dirname(os.path.abspath(__file__))

        model_dir = os.path.join(current_folder, '../downloads/models/pose')
        self.tflite_path = os.path.join(model_dir, tflite_model)
        if not os.path.exists(self.tflite_path):
            raise FileExistsError(f'cannot find tflite model [{self.tflite_path}]')

        video_dir = os.path.join(current_folder, '../downloads/media/movies')
        self.video_path = os.path.join(video_dir, self.video_file)
        if not os.path.exists(self.video_path):
            raise FileExistsError(f'cannot find video [{self.video_path}]')

        Gst.init(argv)

    # @brief main loop execution routine
    def run(self):
        self.mainloop = GLib.MainLoop()

        cmdline =  'filesrc location={:s} ! matroskademux ! vpudec !' \
                       .format(self.video_path)
        cmdline += '  videocrop left=-1 right=-1 top=-1 bottom=-1 ! '
        # crop for square video format
        cmdline += '  video/x-raw,width={:d},height={:d} ! ' \
                       .format(self.VIDEO_INPUT_HEIGHT, self.VIDEO_INPUT_HEIGHT)
        cmdline += '  imxvideoconvert_g2d videocrop-meta-enable=true ! '
        cmdline += '  video/x-raw,width={:d},height={:d},format=RGBA ! ' \
                       .format(self.VIDEO_INPUT_HEIGHT, self.VIDEO_INPUT_HEIGHT)
        cmdline += '  tee name=t '
        cmdline += 't. ! queue name=thread-nn max-size-buffers=2 leaky=2 ! '
        cmdline += '  imxvideoconvert_g2d ! '
        cmdline += '  video/x-raw,width={:d},height={:d},format=RGBA ! ' \
                      .format(self.MODEL_INPUT_WIDTH, self.MODEL_INPUT_HEIGHT)
        cmdline += '  videoconvert ! video/x-raw,format=RGB ! '
        cmdline += '  tensor_converter ! '
        cmdline += self.tensor_transform
        cmdline += '  tensor_filter framework=tensorflow-lite model={:s} {:s} ! ' \
                       .format(self.tflite_path, self.tensor_filter_custom)
        cmdline += '  tensor_sink name=tensor_sink '
        cmdline += 't. ! queue name=thread-img max-size-buffers=2 ! '
        cmdline += '  videoconvert ! cairooverlay name=cairooverlay ! '
        cmdline += '  autovideosink'

        print(cmdline)

        self.pipeline = Gst.parse_launch(cmdline);

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

        # run main loop until EOS or error reported
        self.mainloop.run()

        self.running = False
        self.pipeline.set_state(Gst.State.NULL)

        bus.remove_signal_watch()


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
                np_kpts = np.frombuffer(info.data, dtype = np.float32) \
                          .reshape(self.MODEL_KEYPOINT_SIZE, -1) \
                          .copy()

                # rescale normalized keypoints (x,y) per video resolution
                np_kpts[:, self.MODEL_KEYPOINT_INDEX_X] *= \
                    self.VIDEO_INPUT_CROPPED_WIDTH
                np_kpts[:, self.MODEL_KEYPOINT_INDEX_Y] *= \
                    self.VIDEO_INPUT_CROPPED_HEIGHT

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


if __name__ == '__main__':
    video_file = 'Conditioning_Drill_1-_Power_Jump.webm.480p.vp9.webm'
    video_dims = (854, 480)
    example = PoseExample(video_file, video_dims, sys.argv[1:])
    example.run()


