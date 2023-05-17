#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2023 NXP

import math
import numpy as np
import os


class FNModel:

    def __init__(self, model_directory, vela=False):

        """Helper class for DeepFace-emotion model.

        model_directory: directory where tflite model is located
        vela: use vela version of the model
        """
        # Face detection definitions
        self.MODEL_DEEPFACE_WIDTH = 48
        self.MODEL_DEEPFACE_HEIGHT = 48
        self.MODEL_DEEPFACE_NUM_CLASSES = 7
        self.MODEL_DEEPFACE_CLASSES = ['angry', 'disgust', 'fear', 'happy', 'sad', 'surprise', 'neutral']

        # model location
        if not vela:
            name = 'emotion_uint8_float32.tflite'
        else:
            name = 'emotion_uint8_float32_vela.tflite'
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
        return (self.MODEL_DEEPFACE_HEIGHT, self.MODEL_DEEPFACE_WIDTH, 1)

    def get_model_output_shape(self):
        """Get dimensions of model output tensors (list).
        """
        return [(1, self.MODEL_DEEPFACE_NUM_CLASSES)]

    def predict_emotion(self, prediction):
        """Find the predicted emotion.

        prediction: output tensor of the emotion detection model
        prediction contains a confidence between 0 and 1 for each emotion
        returns: tuple (name, confidence [0, 1.0])
        """

        if not self.check_output_format(prediction):
            raise ValueError(f'prediction format error:\n {prediction}')
        # The predicted emotion is the one with the highest confidence value
        max_prediction = prediction.argmax()
        best = (self.MODEL_DEEPFACE_CLASSES[max_prediction], prediction[max_prediction])
        return best

    def check_output_format(self, prediction):
        """Check consistency of prediction array with the model.

        prediction: output vector to be checked
        """
        shape = prediction.shape
        type = prediction.dtype

        if len(shape) == 1 and \
                shape[0] == self.MODEL_DEEPFACE_NUM_CLASSES and \
                type == np.float32:
            return True
        else:
            return False
