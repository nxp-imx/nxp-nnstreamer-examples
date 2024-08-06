/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef POSE_CUSTOM_POSE_DECODER_H_
#define POSE_CUSTOM_POSE_DECODER_H_

#include <string>
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>

#include "logging.hpp"

typedef struct {
  int kptSize = 17;
  int yIndex = 0;
  int xIndex = 1;
  int scoreIndex = 2;
  float scoreThreshold = 0.4;
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
} DecoderData;


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data);


void drawCallback(GstElement* overlay,
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