/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_CUSTOM_FACE_AND_POSE_DECODER_H_
#define CPP_CUSTOM_FACE_AND_POSE_DECODER_H_

#include <string>
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>
#include <vector>

#include "logging.hpp"

/* Face detection constants */
#define MODEL_UFACE_NUMBER_BOXES              100
#define NUM_BOX_DATA                          6
#define NUMBER_OF_COORDINATES                 4
#define MODEL_UFACE_CLASSIFICATION_THRESHOLD  0.7f
#define MODEL_UFACE_NUMBER_MAX                15

/* Pose detection constants */
#define KPT_SIZE                              17
#define Y_INDEX                               0
#define X_INDEX                               1
#define SCORE_INDEX                           2
#define SCORE_THRESHOLD                       0.4f


typedef struct {
  std::vector<int> selectedBoxes;
  int bufferSize = NUM_BOX_DATA * MODEL_UFACE_NUMBER_BOXES;
  int faceCount = 0;
  int inputDim;
} FaceData;


typedef struct {
  float npKpts[17][3];
  std::string kptLabels[17] = {
      "nose", "left_eye", "right_eye", "left_ear",
      "right_ear", "left_shoulder", "right_shoulder",
      "left_elbow", "right_elbow", "left_wrist", "right_wrist",
      "left_hip", "right_hip", "left_knee", "right_knee", "left_ankle",
      "right_ankle"};
  int kptConnect[17][3] = {
      {1, 2, -1}, {0, 3, -1}, {0, 4, -1}, {1, -1, -1}, {2, -1, -1},
      {6, 7, 11}, {5, 8, 12}, {5, 9, -1}, {6, 10, -1}, {7, -1, -1},
      {8, -1, -1}, {5, 12, 13}, {6, 11, 14}, {11, 15, -1},
      {12, 16, -1}, {13, -1, -1}, {14, -1, -1}};
  int inputDim;
} PoseData;


void newDataPoseCallback(GstElement* element,
                         GstBuffer* buffer,
                         gpointer user_data);


void drawPoseCallback(GstElement* overlay,
                      cairo_t* cr,
                      guint64 timestamp,
                      guint64 duration,
                      gpointer user_data);


void newDataFaceCallback(GstElement* element,
                         GstBuffer* buffer,
                         gpointer user_data);


void drawFaceCallback(GstElement* overlay,
                      cairo_t* cr,
                      guint64 timestamp,
                      guint64 duration,
                      gpointer user_data);


typedef struct {
  int size;
  float* bufferFP32;
} BufferInfo;


BufferInfo getTensorInfo(GstBuffer* buffer, int tensorIndex);


void checkNumTensor(GstBuffer* buffer, int numTensor);

#endif