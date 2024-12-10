/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef FACE_CUSTOM_FACE_DECODER_H_
#define FACE_CUSTOM_FACE_DECODER_H_

#include <string>
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>
#include <vector>

#include "logging.hpp"

const int MODEL_UFACE_NUMBER_BOXES                = 100;
const int NUM_BOX_DATA                            = 6;
const int NUMBER_OF_COORDINATES                   = 4;
const float MODEL_UFACE_CLASSIFICATION_THRESHOLD  = 0.7f;
const int MODEL_UFACE_NUMBER_MAX                  = 15;
const int CAMERA_INPUT_WIDTH                      = 640;
const int CAMERA_INPUT_HEIGHT                     = 480;

typedef struct {
  std::vector<int> selectedBoxes;
  int bufferSize = NUM_BOX_DATA * MODEL_UFACE_NUMBER_BOXES;
  int faceCount = 0;
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