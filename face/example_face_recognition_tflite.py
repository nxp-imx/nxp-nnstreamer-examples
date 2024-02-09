#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

import argparse
import cairo
from facedetectpipe import FaceDetectPipe, SecondaryPipe
import gi
import logging
import facenet
import math
import numpy as np
import re
import os
import sys
import ultraface

gi.require_version('Gst', '1.0')
gi.require_version('GstApp', '1.0')
gi.require_version('GstVideo', '1.0')
from gi.repository import Gst, GLib, GstApp, GstVideo  # noqa

python_path = os.path.join(
    os.path.dirname(
        os.path.abspath(__file__)),
    '../common/python')
sys.path.append(python_path)
from imxpy.imx_dev import Imx, SocId  # noqa


class FaceRecoPipe(FaceDetectPipe):

    def __init__(self, camera_device, video_resolution=(640, 480),
                 video_fps=30, flip=False, secondary_pipe=None, model_directory=None):
        """Subclass FaceDetectPipe.

        Derived class implements specific functions relevant to this example:
        - override primary pipeline cairo overlay implementation
        - override handling of secondary pipeline output tensors (FaceNet)
        - add basic UI control for video overlay

        camera_device: camera device node to be used as pipeline source
        video_resolution: camera video stream resolution
        video_fps: camera video stream frame rate
        secondary_pipe: instance of secondary pipeline for FaceNet operation
        model_directory: absolute path for face detection model
        """
        super(
            FaceRecoPipe,
            self).__init__(
            camera_device,
            video_resolution,
            video_fps,
            flip,
            secondary_pipe,
            model_directory)

        # reference to secondary pipe FaceNet model instance
        self.facenet = self.secondary_pipe.model
        self.mface_db = self.facenet.db_load()

        # UI
        self.ui = UI(self.ui_create_db_entry_cb, self.ui_quit)

    def handle_secondary_output(self, buffer, boxes, index):
        """Override parent implementation to cater for face recognition output.

        buffer: secondary pipeline inference output buffer (embedding)
        boxes: array of boxes detected by primary pipeline
        index: index in boxes array used to compute buffer
        """

        # No faces detected
        if buffer is None or boxes is None:
            self.ui.publish(None, None)
            return

        # extract single output tensor from GStreamer buffer
        dims = self.facenet.get_model_output_shape()
        assert buffer.n_memory() == len(dims)

        # tensor buffer #0
        mem_embedding = buffer.peek_memory(0)
        result, info = mem_embedding.map(Gst.MapFlags.READ)

        dims0 = dims[0]
        num = math.prod(dims0)
        assert info.size == num * np.dtype(np.float32).itemsize

        if result:
            # convert buffer to embedding numpy array
            embedding = np.frombuffer(info.data, dtype=np.float32) \
                .reshape(dims0)[0]

            db = self.mface_db
            name, distance = self.facenet.db_search_match(db, embedding)

            # First face results:
            #  - initialize aggregated results list
            #  - cache last box0 embedding for UI usage (db record saving)
            if index == 0:
                self.embedding0 = embedding.copy()
                self.ui_results = []

            mem_embedding.unmap(info)

            box = boxes[index]
            entry = (box, name, distance)
            self.ui_results += [entry]

            # Last face, publish aggregated results to UI
            total = len(boxes)
            if (index + 1 == total):
                self.ui.publish(self.ui_results, self.embedding0)

        else:
            logging.error('could not map embedding GstBuffer')
            self.ui.publish(None, None)

    def display_cairo_overlay_draw_cb(
            self, overlay, context, timestamp, duration):
        """Override FaceDetectPipe implementation to implement UI specifics.

        Default overlay callback is changed to implement specificities of this
        example:
        - detected name affixed to face bounding box
        - option to enter a name for first detected box to create a new entry
          in embeddings database.
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

        for box, name, distance in results:
            if state == UIState.EDIT:
                if index == 0:
                    context.set_source_rgb(0, 1, 0)
                elif index == 1:
                    context.set_source_rgb(1, 0, 0)
            elif index == 0:
                context.set_source_rgb(1, 0, 0)

            w = box[2] - box[0]
            h = box[3] - box[1]
            context.rectangle(box[0], box[1], w, h)
            context.move_to(box[0], box[1] + h + 20)
            if name is None:
                name = 'Unknown'
            context.show_text(f'{name} ({distance:.3f})')

            context.stroke()
            index += 1

    def ui_create_db_entry_cb(self, name):
        """Callback from UI to create a new embedding entry in database.
        """
        embed0 = self.ui.get_embed0()
        if embed0 is None:
            logging.warning('no face currently detected')
            return
        self.facenet.db_save_record(name, embed0)
        self.facenet.db_add_record(self.mface_db, name, embed0)
        logging.info(f'database entry {name} created')

    def ui_quit(self):
        """Callback from UI to quit application.
        """
        self.mainloop.quit()

    def timeout_function(self):
        """Override FaceDetectPipe implementation to interface UI.
        """
        restart = True
        c = sys.stdin.read(1)
        if len(c) > 0:
            restart = self.ui.inject_key(c)

        return restart


class UIState:
    """UI states definition.

    WARMUP: pipeline stalled pending ML accelerators warmup
    IDLE: standard operation, no face name edition in progress
    EDIT: editor mode, user entering face name
    """
    WARMUP = 0
    IDLE = 1
    EDIT = 2


class UI:

    def __init__(self, create_entry_cb, quit_cb):
        """Handle basic UI rendered in video overlay.

        create_entry_cb: callback to be invoked upon new embedding entry
            creation in the database
        quit_cb: callback to be invoked when user quits application
        """
        self.state = UIState.WARMUP
        self.editor = ''
        self.create_entry_cb = create_entry_cb
        self.quit_cb = quit_cb
        self.detections = None
        self.embd0 = None

    def get_state(self):
        """Report UI state.
        """
        return self.state

    def inject_key(self, c):
        """Handle keypress to implement face entry creation in database.

        c: character value detected from keypress
        """
        restart = True
        if self.state == UIState.WARMUP:
            if c == '\x1b':  # escape
                self.quit_cb()
                restart = False
        elif self.state == UIState.IDLE:
            if c == '\x1b':  # escape
                self.quit_cb()
                restart = False
            elif c == '\x0a':  # enter (Line Feed)
                self.state = UIState.EDIT
        elif self.state == UIState.EDIT:
            if c == '\x1b':  # escape
                self.editor = ''
                self.state = UIState.IDLE
            elif c == '\x7f':  # backspace (DEL)
                if len(self.editor) >= 1:
                    self.editor = self.editor[:-1]
                else:
                    self.state = UIState.IDLE
            elif c == '\x0a':  # enter (Line Feed)
                if len(self.editor) > 0:
                    self.create_entry_cb(self.editor)
                self.state = UIState.IDLE
                self.editor = ''
            elif len(self.editor) < 32:
                c = re.sub('[^\\w_-]', '_', c)
                self.editor += c

        return restart

    def get_display_string(self):
        """Report UI string to be inserted by display overlay.
        """
        if self.state == UIState.WARMUP:
            return 'Warm up in progress - please wait'
        elif self.state == UIState.IDLE:
            return 'Press <ENTER> in console to edit name of the current face'
        elif self.state == UIState.EDIT:
            return f'Enter name: {self.editor}_'
        else:
            return 'Nothing to display!'

    def publish(self, detections, embed0):
        """Publish results from pipeline into UI for use from overlay.

        detections: list of (box, name, distance) tuples
          box: (x1, y1, x2, y2) tuple
          name: matching name or None
          distance: [0-1]
        embed0: box0 (first one detected) embedding
        """
        self.detections = detections
        self.embed0 = embed0

        # On first result, leave warmup state
        if self.state == UIState.WARMUP:
            self.state = UIState.IDLE

    def get_detections(self):
        """ Get detections (box, name, confidence) array coming from pipeline.
        """
        if self.detections is None:
            return []
        else:
            return self.detections.copy()

    def get_embed0(self):
        """ Get #0 embedding detected by pipeline.
        """
        if self.embed0 is None:
            return None
        else:
            return self.embed0.copy()


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

    # pipelines parameters
    camera_device = args.camera_device
    flip = args.mirror
    vr = (640, 480)
    fps = 30

    # secondary pipeline uses FaceNet model
    pwd = os.path.dirname(os.path.abspath(__file__))
    models_dir = os.path.join(pwd, '../downloads/models/face')
    mface_db_dir = os.path.join(pwd, 'facenet_db')

    has_ethosu = imx.has_npu_ethos()
    model = facenet.FNModel(models_dir, mface_db_dir, vela=has_ethosu)
    secondary = SecondaryPipe(model, video_resolution=vr, video_fps=fps)

    # main pipeline for face detection
    pipe = FaceRecoPipe(camera_device=camera_device, video_resolution=vr,
                        video_fps=fps, flip=flip, secondary_pipe=secondary,
                        model_directory=models_dir)
    pipe.run()
