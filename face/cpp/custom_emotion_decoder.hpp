/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef FACE_CUSTOM_FACE_DECODER_H_
#define FACE_CUSTOM_FACE_DECODER_H_

#include <string>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>
#include <vector>

#include "logging.hpp"

const int MODEL_UFACE_NUMBER_NMS_BOXES            = 100;
const int NUMBER_OF_INFOS                         = 6;
const int NUMBER_OF_COORDINATES                   = 4;
const float MODEL_UFACE_CLASSIFICATION_THRESHOLD  = 0.7f;
const int MODEL_UFACE_NUMBER_MAX                  = 15;
const int CAMERA_INPUT_WIDTH                      = 640;
const int CAMERA_INPUT_HEIGHT                     = 480;


typedef struct {
  std::vector<std::string> emotions;
  std::vector<float> values;
  std::vector<int> boxes;
} EmotionData;


typedef struct {
  int SubFaceCount = 0;
  std::vector<int> faceBoxes;
  std::vector<int> emotionBoxes;
  std::string modelDeepfaceClasses[7] = {"angry", "disgust", "fear", "happy", "sad", "surprise", "neutral"};
  GstBuffer *imagesBuffer;
  bool subActive = false;
  GstElement *appSrc;
  GstElement *videocrop;
  EmotionData result;
} DecoderData;


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data);


void secondaryNewDataCallback(GstElement* element,
                              GstBuffer* buffer,
                              gpointer user_data);


GstFlowReturn sinkCallback(GstAppSink* appsink, gpointer user_data);


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