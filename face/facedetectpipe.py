#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

import argparse
import cairo
import gi
import logging
import math
import numpy as np
import termios
import ultraface
import os
import signal
import sys

gi.require_version('Gst', '1.0')
gi.require_version('GstApp', '1.0')
gi.require_version('GstVideo', '1.0')
from gi.repository import Gst, GLib, GstApp, GstVideo  # noqa

python_path = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                           '../common/python')
sys.path.append(python_path)
from imxpy.imx_dev import Imx, SocId  # noqa
from imxpy.common_utils import GstVideoImx, store_vx_graph_compilation  # noqa


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
        try :
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
        if not self.background_execution :
            termios.tcsetattr(sys.stdin, termios.TCSANOW, self.old_attributes)


class Pipe:

    def __init__(self, name='pipeline'):
        """Provide GStreamer pipeline class with common helper functions.

        name: name associated to underlying GStreamer pipeline
        """
        self.pipe_name = name

        # This env needs to be set early
        dir = os.environ.get('GST_DEBUG_DUMP_DOT_DIR')
        if dir is None:
            dir = os.getcwd()
            os.environ['GST_DEBUG_DUMP_DOT_DIR'] = dir

        if not Gst.is_initialized():
            Gst.init(None)

        self.imx = Imx()
        store_vx_graph_compilation(self.imx)

    def dump_gst_dot_file(self, name=None):
        """Dump GStreamer .dot file.

        name: base name of the .dot file to be generated
        """
        if name is None:
            name = self.pipe_name
        dir = os.environ.get('GST_DEBUG_DUMP_DOT_DIR')
        path = os.path.join(dir, f'{name}.dot')
        if os.path.exists(path):
            os.remove(path)
        logging.debug(f'Dump gst .dot file {path}')
        Gst.debug_bin_to_dot_file(self.pipeline, Gst.DebugGraphDetails.ALL,
                                  name)
        return path

    def on_bus_message(self, bus, message, name=None):
        """Generic pipeline bus message handler.

        message: pipeline message passed to the bus
        name: pipeline name that originated the message
        """
        if name is None:
            name = self.pipe_name
        type = message.type
        if type == Gst.MessageType.EOS:
            logging.info('[%s] bus eos', name)
        elif type == Gst.MessageType.ERROR:
            error, debug = message.parse_error()
            logging.warning('[%s] bus error %s %s', name, error.message, debug)
        elif type == Gst.MessageType.WARNING:
            error, debug = message.parse_warning()
            logging.warning('[%s] bus warning %s %s', name, error.message,
                            debug)
        elif type == Gst.MessageType.STREAM_START:
            logging.info('[%s] bus start', name)
            self.dump_gst_dot_file()

    def nns_tfliter_custom_options(self):
        """Report custom options to use for nnstreamer tensor filter.
        """
        if self.imx.has_npu_vsi():
            opts = ('custom=Delegate:External,'
                    'ExtDelegateLib:libvx_delegate.so')
        elif self.imx.has_npu_ethos():
            opts = ('custom=Delegate:External,'
                    'ExtDelegateLib:libethosu_delegate.so')
        else:
            opts = ''
        return opts


class SecondaryPipe(Pipe):

    def __init__(self,
                 secondary_model,
                 video_resolution=(640, 480),
                 video_fps=30):
        """Secondary GStreamer pipeline applying 2nd model on detected faces.

        Secondary GStreamer pipeline to be fed with video stream and a set of
        detected faces to be individually cropped and fed to an additional ML
        model

        secondary_model: instance of ML model to be used by secondary pipeline
        video_resolution: input video resolution
        video_fps: input video framerate
        """
        super(SecondaryPipe, self).__init__(name='secondary')

        self.model = secondary_model
        self.video_input_width = video_resolution[0]
        self.video_input_height = video_resolution[1]
        self.video_rate = video_fps

        vw = self.video_input_width
        vh = self.video_input_height
        vr = self.video_rate

        self.processing_complete_cb = None

        gstvideoimx = GstVideoImx(self.imx)

        # secondary pipeline (face crop + second model) GStreamer definition
        video_caps = ('video/x-raw,'
                      'width={:d},height={:d},framerate={:d}/1,format=YUY2') \
            .format(vw, vh, vr)

        cmdline = 'appsrc name=appsrc_video is-live=true caps={:s} format=3 ' \
            .format(video_caps)
        cmdline += '    emit-signals=false max-buffers=1 leaky_type=2 ! '
        cmdline += '  {:s} ! '.format(video_caps)

        h, w, num_channels = secondary_model.get_model_input_shape()
        if num_channels == 3:
            format = 'RGB'
        elif num_channels == 1:
            format = 'GRAY8'
        cmdline += gstvideoimx.videocrop_to_format('video_crop', w, h, format=format)

        cmdline += '  tensor_converter ! '

        custom_ops = self.nns_tfliter_custom_options()
        cmdline += ('  tensor_filter '
                    'framework=tensorflow-lite model={:s} {:s} ! ') \
            .format(secondary_model.get_model_path(), custom_ops)
        cmdline += ('  tensor_sink '
                    'name=tsink_fr emit-signal=true sync=false qos=false')

        logging.info('sub pipeline: %s', cmdline)
        self.pipeline = Gst.parse_launch(cmdline)

        # pipeline bus and message callback
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message', self.on_secondary_bus_message)

        # video appsrc element
        self.app_src_video = self.pipeline.get_by_name('appsrc_video')

        # videocrop element
        self.videocrop = self.pipeline.get_by_name('video_crop')

        # face recognition tensor_sink element
        tensor_sink_fr = self.pipeline.get_by_name('tsink_fr')
        tensor_sink_fr.connect('new-data',
                               self.secondary_tensor_sink_new_data_cb)

    def start(self):
        """Start the secondary pipeline.
        """
        self.running = True
        self.pipeline.set_state(Gst.State.PLAYING)

    def stop(self):
        """Stop the secondary pipeline.
        """
        self.pipeline.set_state(Gst.State.NULL)
        self.running = False

        self.bus.remove_signal_watch()

    def on_secondary_bus_message(self, bus, message):
        """Secondary pipeline bus message callback.
        """
        self.on_bus_message(bus, message)

    def secondary_tensor_sink_new_data_cb(self, sink, buffer):
        """Callback for secondary pipeline output tensor sink signal.
        """
        if self.processing_complete_cb is not None:
            self.processing_complete_cb(buffer)

    def register_callback_processing_complete(self, complete_cb):
        """Callback registration to be notified of processing complete
        """
        self.processing_complete_cb = complete_cb

    def push_buffer(self, buffer, boxes, index):
        """Push video buffer and metadata into secondary pipeline appsrc

        buffer: GStreamer buffer
        boxes: array of N boxes (N, 4) to be cropped as ROI
               box: x1, y1, x2, y2
        index: index of the box to use from the boxes array
        """
        assert index < len(boxes)
        box = boxes[index]
        vw, vh = self.video_input_width, self.video_input_height

        # configure videocrop for detected face
        ctop = box[1]
        cbottom = vh - box[3]
        cleft = box[0]
        cright = vw - box[2]
        logging.debug('push ROI: top:%d bottom:%d left:%d right:%d', ctop,
                      cbottom, cleft, cright)
        self.videocrop.set_property('top', ctop)
        self.videocrop.set_property('bottom', cbottom)
        self.videocrop.set_property('left', cleft)
        self.videocrop.set_property('right', cright)

        self.app_src_video.push_buffer(buffer)


class FaceDetectPipe(Pipe):

    def __init__(self,
                 camera_device,
                 video_resolution=(640, 480),
                 video_fps=30,
                 secondary_pipe=None,
                 model_directory=None):
        """Main GStreamer pipeline running face detection on video stream.

        It is optionally sequencing a secondary pipeline provided with the
        video and cropping info detected for every face.

        camera_device: camera device node to be used as pipeline source
        video_resolution: camera video stream resolution
        video_fps: camera video stream frame rate
        secondary_pipe: optional instance of secondary pipeline
        model_directory: absolute path for face detection model
        """
        super(FaceDetectPipe, self).__init__(name='main')

        # video input definitions
        self.camera_device = camera_device
        self.video_input_width = video_resolution[0]
        self.video_input_height = video_resolution[1]
        self.video_rate = video_fps

        if not os.path.exists(self.camera_device):
            raise FileExistsError(f'cannot find camera [{self.camera_device}]')

        # models
        if model_directory is None:
            pwd = os.path.dirname(os.path.abspath(__file__))
            model_directory = os.path.join(pwd, '../downloads/models/face')

        has_ethosu = self.imx.has_npu_ethos()
        self.ultraface = ultraface.UFModel(model_directory, vela=has_ethosu)

        # pipelines variables
        self.mainloop = None
        self.pipeline = None
        self.running = False
        self.fd_boxes = np.empty(0, dtype=np.float32)
        self.done = False

        # secondary pipeline variables
        self.secondary_pipe = secondary_pipe
        if secondary_pipe is not None:
            cb = self.secondary_processing_complete_cb
            self.secondary_pipe.register_callback_processing_complete(cb)
        self.sub_active = False
        self.sub_input_buffer = None
        self.sub_face_count = 0

        # main pipeline (face detection) GStreamer definition
        vw = self.video_input_width
        vh = self.video_input_height
        vr = self.video_rate

        ufh, ufw, _ = self.ultraface.get_model_input_shape()

        gstvideoimx = GstVideoImx(self.imx)

        video_caps = ('video/x-raw, '
                      'width={:d},height={:d},framerate={:d}/1,format=YUY2') \
            .format(vw, vh, vr)

        cmdline = 'v4l2src device={:s} ! '.format(self.camera_device)
        cmdline += '  {:s} ! '.format(video_caps)
        cmdline += '  tee name=tvideo '
        cmdline += 'tvideo. ! queue max-size-buffers=1 leaky=2 ! '

        cmdline += gstvideoimx.accelerated_videoscale(ufw, ufh, 'RGB')

        cmdline += '  tensor_converter ! '

        custom_ops = self.nns_tfliter_custom_options()
        cmdline += ('  tensor_filter '
                    'framework=tensorflow-lite model={:s} {:s} ! ') \
            .format(self.ultraface.get_model_path(), custom_ops)
        cmdline += '  tensor_sink name=tsink_fd '
        cmdline += 'tvideo. ! queue max-size-buffers=1 leaky=2 ! '

        # cairo overlay format restricted to BGRx, BGRA, RGB16
        cmdline += gstvideoimx.accelerated_videoscale(format='RGB16')

        cmdline += '  cairooverlay name=cairooverlay ! '
        cmdline += '  autovideosink sync=false '.format(vw, vh)
        cmdline += 'tvideo. ! queue max-size-buffers=1 leaky=2 ! '
        cmdline += ('  appsink '
                    'name=appsink_video sync=false max-buffers=1 drop=true '
                    'emit-signals=true ')

        signal.signal(signal.SIGINT, self.sigint_handler)

        logging.info('main pipeline: %s', cmdline)
        self.pipeline = Gst.parse_launch(cmdline)

        # pipeline bus and message callback
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()
        self.bus.connect('message', self.on_main_bus_message)

        # tensor sink signal : new-data callback
        tensor_sink_fd = self.pipeline.get_by_name('tsink_fd')
        tensor_sink_fd.connect('new-data',
                               self.face_detect_tensor_sink_new_data_cb)

        # cairooverlay signal: draw callback
        overlay = self.pipeline.get_by_name('cairooverlay')
        overlay.connect('draw', self.display_cairo_overlay_draw_cb)

        # video appsink callback
        app_sink_video = self.pipeline.get_by_name('appsink_video')
        app_sink_video.connect("new-sample", self.video_app_sink_new_sample_cb)

    def on_main_bus_message(self, bus, message):
        """Main pipeline bus message callback.
        """
        self.on_bus_message(bus, message)

    def face_detect_tensor_sink_new_data_cb(self, sink, buffer):
        """Callback for face detection output tensor sink signal.
        """
        if not self.running:
            return

        dims = self.ultraface.get_model_output_shape()
        assert buffer.n_memory() == len(dims)

        # tensor buffer #0
        buf0 = buffer.peek_memory(0)
        result, info = buf0.map(Gst.MapFlags.READ)

        dims0 = dims[0]
        num = math.prod(dims0)
        assert info.size == num * np.dtype(np.float32).itemsize

        if result:
            out0 = np.frombuffer(info.data, dtype=np.float32) \
                .reshape(dims0)
            selected = self.ultraface.decode_output([out0])
            # rescale boxes per actual input video resolution
            w, h = self.video_input_width, self.video_input_height
            scale = np.array([w, h])
            selected *= np.tile(scale, 2)
            selected = selected.astype(int)
            # XXX: clip to 64x64 min (imxvideoconvert_g2d constraint)
            self.fd_boxes = self.square_boxes(selected, w, h, k=0.8, minwh=64)
        else:
            self.fd_boxes = np.empty(0, dtype=np.float32)

        buf0.unmap(info)

    def display_cairo_overlay_draw_cb(self, overlay, context, timestamp,
                                      duration):
        """Callback to draw from cairooverlay GStreamer element.
        """
        if not self.running:
            return

        boxes = self.fd_boxes
        count = len(self.fd_boxes)

        context.set_source_rgb(0.85, 0, 1)
        context.move_to(14, 14)
        context.select_font_face('Arial', cairo.FONT_SLANT_NORMAL,
                                 cairo.FONT_WEIGHT_NORMAL)
        context.set_font_size(11.0)
        context.show_text(f'Faces detected: {count}')

        if count == 0:
            return

        context.set_source_rgb(1, 0, 0)
        context.set_line_width(1.0)

        for box in boxes:
            w = box[2] - box[0]
            h = box[3] - box[1]
            context.rectangle(box[0], box[1], w, h)

        context.stroke()

    def video_app_sink_new_sample_cb(self, sink):
        """Callback for face detection output tensor sink signal.
        """
        sample = sink.pull_sample()
        if not isinstance(sample, Gst.Sample):
            logging.error('video appsink sample is not a Gst.Sample')
            return Gst.FlowReturn.ERROR

        # if no secondary pipeline, nothing to do
        if self.secondary_pipe is None:
            self.handle_secondary_output(None, None, -1)
            return Gst.FlowReturn.OK

        buffer = sample.get_buffer()

        if (self.sub_active):
            logging.debug('sub pipeline busy - drop')
            return Gst.FlowReturn.OK

        self.sub_boxes = np.copy(self.fd_boxes)
        num = len(self.sub_boxes)
        if (num == 0):
            logging.debug('no faces - drop')
            self.handle_secondary_output(None, None, -1)
            return Gst.FlowReturn.OK

        logging.debug('sub pipeline kick: %d faces %s', num,
                      str(self.sub_boxes))
        self.sub_active = True
        self.sub_face_count = 0
        self.sub_input_buffer = buffer

        self.secondary_pipe.push_buffer(buffer, self.sub_boxes, 0)

        return Gst.FlowReturn.OK

    def secondary_processing_complete_cb(self, output):
        """Callback from secondary pipeline when it has completed processing.

        output: GStreamer buffer output from secondary pipeline (tensor sink)
        """
        count = self.sub_face_count

        self.handle_secondary_output(output, self.sub_boxes, count)

        count += 1
        total = len(self.sub_boxes)

        if (count < total):
            logging.debug('continue')
            input = self.sub_input_buffer
            self.sub_face_count = count

            self.secondary_pipe.push_buffer(input, self.sub_boxes, count)
        else:
            logging.debug('halt')

            self.sub_input_buffer = None
            self.sub_boxes = None
            self.sub_face_count = 0
            self.sub_active = False

    def handle_secondary_output(self, buffer, boxes, index):
        """Handle output from the secondary pipe processing (appsink).

        buffer: GStreamer buffer with output tensor(s)
                None if no faces detected
        boxes: array of N boxes (N, 4) to be cropped as ROI
               box: (x1, y1, x2, y2)
               None if no faces detected
        index: index of the box to use from the boxes array
        """
        # do something usefull with tensor(s) outputs...
        pass

    def square_boxes(self, boxes, videow, videoh, k=1.0, minwh=-1):
        """Transform rectangular to square box using greater length.

        boxes: array (N,box) with box (x1, y1, x2, y2)
        videow: video width (max square edge)
        videoh: video height (max square edge)
        k: scaling factor (1.0 means unchanged)
        minwh: minimum width and height for resulting box
        return: array of squared boxes
        """
        res = np.zeros(len(boxes) * 4, dtype=int).reshape(len(boxes), 4)
        count = 0
        for box in boxes:
            w = (box[2] - box[0] + 1)
            h = (box[3] - box[1] + 1)
            d = int(max(w, h)) * k
            cx = int((box[0] + box[2]) / 2)
            cy = int((box[1] + box[3]) / 2)
            d = min(d, min(videow, videoh))
            if minwh > 0:
                d = max(d, minwh)
            d2 = int(d / 2)
            if (cx + d2) >= videow:
                cx = videow - d2 - 1
            if (cx - d2) < 0:
                cx = d2
            if (cy + d2) >= videoh:
                cy = videoh - d2 - 1
            if (cy - d2) < 0:
                cy = d2
            res[count] = [cx - d2, cy - d2, cx + d2, cy + d2]
            assert (res[count][0] >= 0) and (res[count][0] < videow)
            assert (res[count][1] >= 0) and (res[count][1] < videoh)
            assert (res[count][2] >= 0) and (res[count][2] < videow)
            assert (res[count][3] >= 0) and (res[count][3] < videoh)
            count += 1
        return res

    def start(self):
        """Start the pipeline(s).
        """
        self.running = True
        if self.secondary_pipe is not None:
            self.secondary_pipe.start()
        self.pipeline.set_state(Gst.State.PLAYING)

    def stop(self):
        """Stop the pipeline(s).
        """
        self.pipeline.set_state(Gst.State.NULL)
        if self.secondary_pipe is not None:
            self.secondary_pipe.stop()
        self.running = False

        self.bus.remove_signal_watch()

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

    def run(self):
        """Run pipeline until user exits or interrupted.
        """
        stdin = StdInHelper()

        logging.info('Press <ESC> to quit.')

        stdin.set_attr_non_blocking()
        self.start()

        # create and run GLib main loop
        self.mainloop = GLib.MainLoop()
        GLib.timeout_add(100, self.timeout_function)
        self.mainloop.run()

        self.stop()
        stdin.set_attr_restore()


if __name__ == '__main__':

    imx = Imx()
    soc = imx.id()
    if not (soc == SocId.IMX8MP or soc == SocId.IMX93):
        name = imx.name()
        raise NotImplementedError(f'Platform not supported [{name}]')

    if soc == SocId.IMX8MP:
        default_camera = '/dev/video3'
    else:
        default_camera = '/dev/video0'

    parser = argparse.ArgumentParser(description='Face Identification')
    parser.add_argument('--camera_device',
                        type=str,
                        help='camera device node',
                        default=default_camera)
    args = parser.parse_args()

    format = '%(asctime)s.%(msecs)03d %(levelname)s:\t%(message)s'
    datefmt = '%Y-%m-%d %H:%M:%S'
    logging.basicConfig(level=logging.INFO, format=format, datefmt=datefmt)

    # pipeline parameters - no secondary pipeline
    camera_device = args.camera_device
    vr = (640, 480)
    fps = 30
    secondary = None

    pipe = FaceDetectPipe(camera_device=camera_device,
                          video_resolution=vr,
                          video_fps=fps,
                          secondary_pipe=secondary)
    pipe.run()
