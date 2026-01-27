#!/usr/bin/env python3
#
# Copyright 2022-2023, 2025-2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

import numpy as np
import os
from imxpy.imx_dev import Imx  # noqa

class UFModel:

    def __init__(self, model_directory, max_faces=16,
                 classification_threshold=0.7):
        """Helper class for UltraFace model.

        model_directory: directory where tflite model is located
        max_faces: maximum number of detected faces to be reported
        classification_threshold: threshold for classifier confidence
        """

        # Face detection definitions
        self.MODEL_UFACE_WIDTH = 320
        self.MODEL_UFACE_HEIGHT = 240
        self.MODEL_UFACE_NUMBER_NMS_BOXES = 100

        # Check for model-specific backend first, then fall back to global BACKEND
        self.backend = os.getenv('BACKEND_ULTRAFACE', os.getenv('BACKEND', 'CPU'))

        self.MODEL_UFACE_CLASSIFICATION_THRESHOLD = classification_threshold

        # Limit max number of face detections
        self.MODEL_UFACE_NUMBER_MAX = max_faces

        # model location
        self.imx = Imx()
        if self.backend == 'NPU':
            if self.imx.has_npu_ethos():
                name = 'ultraface_slim_uint8_float32_vela.tflite'
            elif self.imx.has_npu_neutron():
                if self.imx.is_imx95():
                    name = 'ultraface_slim_uint8_float32_imx95.tflite'
                else: #imx952 device
                    name = 'ultraface_slim_uint8_float32_imx952.tflite'
            else: #imx8mp device
                name = 'ultraface_slim_uint8_float32.tflite'
        else:  # backend = CPU
            name = 'ultraface_slim_uint8_float32.tflite'

        self.tflite_model = os.path.join(model_directory, name)

        if not os.path.exists(self.tflite_model):
            raise FileExistsError(f'cannot find model [{self.tflite_model}]')

    def get_backend(self):
        """Get backend type.
        """
        return self.backend

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
        return [(self.MODEL_UFACE_NUMBER_NMS_BOXES, 6)]

    def decode_output(self, output):
        """Decode model output tensor.

        output: list of output tensors to decode
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

        boxes = out0[..., 2:]
        scores = out0[..., 1]
        selected = scores > self.MODEL_UFACE_CLASSIFICATION_THRESHOLD
        boxes = boxes[selected]

        selected = boxes[:self.MODEL_UFACE_NUMBER_MAX, ...]

        return selected
