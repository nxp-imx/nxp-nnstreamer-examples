#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

import numpy as np
import os
import sys


class UFModel:

    def __init__(self, model_directory, max_faces=16, has_post_process=True,
                 classification_threshold=0.7, nms_iou_threshold=0.5,
                 vela=False):
        """Helper class for UltraFace model.

        model_directory: directory where tflite model is located
        max_faces: maximum number of detected faces to be reported
        has_post_process: use model with embedded post-process
            (box decoding, nms)
        classification_threshold: threshold for classifier confidence
        nms_iou_threshold: threshold for NMS IoU dupe verdict
        vela: use vela version of the model
        """

        # Face detection definitions
        self.MODEL_UFACE_WIDTH = 320
        self.MODEL_UFACE_HEIGHT = 240
        # No postprocessing
        self.MODEL_UFACE_NUMBER_BOXES = 4420
        self.MODEL_UFACE_NMS_IOU_THRESHOLD = nms_iou_threshold

        # post processing : box decoding + NMS
        self.MODEL_UFACE_NUMBER_NMS_BOXES = 100

        self.MODEL_UFACE_CLASSIFICATION_THRESHOLD = classification_threshold

        # Limit max number of face detections
        self.MODEL_UFACE_NUMBER_MAX = max_faces

        # model location
        self.has_post_process = has_post_process
        if has_post_process:
            if not vela:
                name = 'ultraface_slim_uint8_float32.tflite'
            else:
                name = 'ultraface_slim_uint8_float32_vela.tflite'
        else:
            name = 'version-slim_input_uint8_output_float32.tflite'
        self.tflite_model = os.path.join(model_directory, name)

        if not os.path.exists(self.tflite_model):
            raise FileExistsError(f'cannot find model [{self.tflite_model}]')

    def get_model_path(self):
        """Get full path to model file.
        """
        return self.tflite_model

    def get_model_input_shape(self):
        """Get dimensions of model input tensor.
        """
        return (self.MODEL_UFACE_HEIGHT, self.MODEL_UFACE_WIDTH, 3)

    def get_model_output_shape(self):
        """Get dimensions of model output tensors (list).
        """
        if self.has_post_process:
            return [(self.MODEL_UFACE_NUMBER_NMS_BOXES, 6)]
        else:
            return [(1, self.MODEL_UFACE_NUMBER_BOXES, 6)]

    def iou(self, box0, box1):
        """Compute input boxes IoU.

        box0:  (x1, y1, x2, y2)
        box1:  (x1, y1, x2, y2)
        return: computed IoU
        """
        assert box0[0] <= box0[2]
        assert box0[1] <= box0[3]
        assert box1[0] <= box1[2]
        assert box1[1] <= box1[3]

        x1i = max(box0[0], box1[0])
        y1i = max(box0[1], box1[1])
        x2i = min(box0[2], box1[2])
        y2i = min(box0[3], box1[3])
        # boxes individual and intersection surfaces
        si = max(0, x2i - x1i + 1) * max(0, y2i - y1i + 1)
        s0 = (box0[2] - box0[0] + 1) * (box0[3] - box0[1] + 1)
        s1 = (box1[2] - box1[0] + 1) * (box1[3] - box1[1] + 1)

        val = si / (s0 + s1 - si)
        assert val >= 0 and val <= 1

        return val

    def nms(self, boxes, scores, max_output_size, iou_threshold=0.5):
        """Execute NMS procedure on boxes list.

        boxes: array (N, 4) of N boxes (x1, y1, x2, y2)
        scores: array of N individual boxes score
        max_output_size: max number of boxes to be returned
        iou_threshold: IoU threshold for boxes duplication verdict
        return: array of remaining boxes after NMS filtering
        """
        result = np.zeros((max_output_size, 4), dtype=float)
        count = 0

        sorted = np.argsort(scores, axis=0)
        sorted = sorted[-1::-1]
        _boxes = np.take(boxes, sorted, axis=0)

        while len(_boxes) > 0:
            if count >= max_output_size:
                return result
            result[count] = _boxes[0]
            count += 1

            dupe = [0]
            for i in range(1, len(_boxes)):
                if self.iou(_boxes[0], _boxes[i]) > iou_threshold:
                    dupe += [i]
            _boxes = np.delete(_boxes, dupe, axis=0)

        return result[:count, ...]

    def decode_output(self, output):
        """Decode model output tensor.

        output: list of output tensors to decode
            output0 (no post processing): tensor (100,6)
               100 boxes filtered by model built-in NMS operator
            output0 (no post processing): tensor (1,4480,6)
               4480 raw boxes
            With and without post processing, box format
               [...,0:2] classifier output
               [...,2:6] decoded box (x1,y2,x2,y2)
        return: array (N, 4)
           N boxes (x1,y2,x2,y2) detected
           Normalized coordinates range [0, 1]
        """
        assert len(output) == 1
        out0 = output[0]
        assert out0.dtype == np.float32
        assert out0.shape == self.get_model_output_shape()[0]

        if self.has_post_process:
            boxes = out0[..., 2:]
            scores = out0[..., 1]
            selected = scores > self.MODEL_UFACE_CLASSIFICATION_THRESHOLD
            boxes = boxes[selected]

            selected = boxes[:self.MODEL_UFACE_NUMBER_MAX, ...]
        else:
            boxes = out0[..., 2:]
            scores = out0[..., :2]
            boxes = boxes[0]
            scores = scores[0, :, 1]
            selected = scores > self.MODEL_UFACE_CLASSIFICATION_THRESHOLD
            boxes = boxes[selected]
            scores = scores[selected]

            selected = self.nms(boxes, scores,
                                self.MODEL_UFACE_NUMBER_MAX,
                                self.MODEL_UFACE_NMS_IOU_THRESHOLD)

        return selected
