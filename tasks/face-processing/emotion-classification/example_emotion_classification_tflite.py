#!/usr/bin/env python3
#
# Copyright 2023-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import cairo

import gi
import logging
import math
import numpy as np
import os
import sys

gi.require_version('Gst', '1.0')
gi.require_version('GstApp', '1.0')
gi.require_version('GstVideo', '1.0')
from gi.repository import Gst, GLib, GstApp, GstVideo  # noqa

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../../../common/python')
sys.path.append(python_path)
from imxpy.imx_dev import Imx, SocId  # noqa

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../common')
sys.path.append(python_path)
from facedetectpipe import FaceDetectPipe, SecondaryPipe # noqa
import deepface # noqa


class EmoDetectPipe(FaceDetectPipe):

    def __init__(self, camera_device, video_resolution=(640, 480),
                 video_fps=30, flip=False, secondary_pipe=None, model_directory=None):
        """Subclass FaceDetectPipe.

        Derived class implements specific functions relevant to this example:
        - override primary pipeline cairo overlay implementation
        - override handling of secondary pipeline output tensors (DeepFace)
        - add basic UI control for video overlay

        camera_device: camera device node to be used as pipeline source
        video_resolution: camera video stream resolution
        video_fps: camera video stream frame rate
        secondary_pipe: instance of secondary pipeline for DeepFace operation
        model_directory: absolute path for face detection model
        """
        super(
            EmoDetectPipe,
            self).__init__(
            camera_device,
            video_resolution,
            video_fps,
            flip,
            secondary_pipe,
            model_directory)

        # reference to secondary pipe DeepFace model instance
        self.deepface = self.secondary_pipe.model

        # UI
        self.ui = UI()

    def handle_secondary_output(self, buffer, boxes, index):
        """Override parent implementation to cater for emotion detection output.

        buffer: secondary pipeline inference output buffer that contains confidence value for each emotion
        boxes: array of boxes detected by primary pipeline
        index: index in boxes array used to compute buffer
        """

        # No faces detected
        if buffer is None or boxes is None:
            self.ui.publish(None)
            return

        # extract single output tensor from GStreamer buffer
        dims = self.deepface.get_model_output_shape()
        assert buffer.n_memory() == len(dims)

        # tensor buffer #0
        mem_prediction = buffer.peek_memory(0)
        result, info = mem_prediction.map(Gst.MapFlags.READ)

        dims0 = dims[0]
        num = math.prod(dims0)
        assert info.size == num * np.dtype(np.float32).itemsize

        if result:
            # convert buffer to prediction numpy array
            prediction = np.frombuffer(info.data, dtype=np.float32) \
                .reshape(dims0)[0]

            emotion, value = self.deepface.predict_emotion(prediction)

            if index == 0:
                self.ui_results = []

            mem_prediction.unmap(info)

            box = boxes[index]
            entry = (box, emotion, value)
            self.ui_results += [entry]

            # Last face, publish aggregated results to UI
            total = len(boxes)
            if (index + 1 == total):
                self.ui.publish(self.ui_results)

        else:
            logging.error('could not map prediction GstBuffer')
            self.ui.publish(None, None)

    def display_cairo_overlay_draw_cb(
            self, overlay, context, timestamp, duration):
        """Override FaceDetectPipe implementation to implement UI specifics.

        Default overlay callback is changed to implement specificities of this
        example:
        - detected emotion affixed to face bounding box
        """

        if not self.running:
            return

        results = self.ui.get_detections()
        total = len(results)

        context.set_source_rgb(0.85, 0, 1)
        context.move_to(14, 14)
        context.select_font_face(
            'Arial',
            cairo.FONT_SLANT_NORMAL,
            cairo.FONT_WEIGHT_NORMAL)
        context.set_font_size(11.0)

        state = self.ui.get_state()
        display = self.ui.get_display_string()
        context.show_text(display)

        if total == 0:
            return

        index = 0
        context.set_line_width(1.0)

        for box, emotion, confidence in results:
            if index == 0:
                context.set_source_rgb(1, 0, 0)

            w = box[2] - box[0]
            h = box[3] - box[1]
            context.rectangle(box[0], box[1], w, h)
            context.move_to(box[0], box[1] + h + 20)

            context.show_text(f'{emotion} ({confidence:.3f})')

            context.stroke()
            index += 1

    def ui_quit(self):
        """Callback from UI to quit application.
        """
        self.mainloop.quit()


class UIState:
    """UI states definition.

    WARMUP: pipeline stalled pending ML accelerators warmup
    IDLE: standard operation
    """

    WARMUP = 0
    IDLE = 1


class UI:

    def __init__(self):
        """Handle basic UI rendered in video overlay.

        quit_cb: callback to be invoked when user quits application
        """
        self.state = UIState.WARMUP
        self.detections = None

    def get_state(self):
        """Report UI state.
        """
        return self.state

    def get_display_string(self):
        """Report UI string to be inserted by display overlay.
        """
        if self.state == UIState.WARMUP:
            return 'Warm up in progress - please wait'
        elif self.state == UIState.IDLE:
            return 'Show your face in front of the camera !'
        else:
            return 'Nothing to display!'

    def publish(self, detections):
        """Publish results from pipeline into UI for use from overlay.

        detections: list of (box, emotion, distance) tuples
          box: (x1, y1, x2, y2) tuple
          emotion: matching emotion
          distance: [0-1]
        """
        self.detections = detections

        # On first result, leave warmup state
        if self.state == UIState.WARMUP:
            self.state = UIState.IDLE

    def get_detections(self):
        """ Get detections (box, emotion, confidence) array coming from pipeline.
        """
        if self.detections is None:
            return []
        else:
            return self.detections.copy()


if __name__ == '__main__':

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

    parser = argparse.ArgumentParser(description='Emotion detection')
    parser.add_argument('--camera_device', '-c', type=str,
                        help='camera device node', default=default_camera)
    parser.add_argument('--mirror', '-m',
                        default=False, action='store_true',
                        help='flip image to display as a mirror')
    args = parser.parse_args()

    format = '%(asctime)s.%(msecs)03d %(levelname)s:\t%(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'
    logging.basicConfig(level=logging.INFO, format=format, datefmt=datefmt)

    # pipelines parameters
    camera_device = args.camera_device
    flip = args.mirror
    vr = (640, 480)
    fps = 30

    # secondary pipeline uses DeepFace model
    pwd = os.path.dirname(os.path.abspath(__file__))
    models_dir = os.path.join(pwd, '../../../downloads/models/face-processing')

    has_ethosu = imx.has_npu_ethos()
    model = deepface.FNModel(models_dir, vela=has_ethosu)
    secondary = SecondaryPipe(model, video_resolution=vr, video_fps=fps)

    # main pipeline for face detection
    pipe = EmoDetectPipe(camera_device=camera_device, video_resolution=vr,
                         video_fps=fps, flip=flip, secondary_pipe=secondary,
                         model_directory=models_dir)
    pipe.run()
