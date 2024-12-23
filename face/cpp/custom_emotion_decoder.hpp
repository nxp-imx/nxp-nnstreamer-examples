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

#define MODEL_UFACE_NUMBER_BOXES              100
#define NUM_BOX_DATA                          6
#define NUMBER_OF_COORDINATES                 4
#define MODEL_UFACE_CLASSIFICATION_THRESHOLD  0.7f
#define MODEL_UFACE_NUMBER_MAX                15
#define CAMERA_INPUT_WIDTH                    640
#define CAMERA_INPUT_HEIGHT                   480


typedef struct {
  std::vector<std::string> emotions;
  std::vector<float> values;
  std::vector<int> boxes;
} EmotionData;


typedef struct {
  int SubFaceCount = 0;
  std::vector<int> faceBoxes;
  int bufferSize = NUM_BOX_DATA * MODEL_UFACE_NUMBER_BOXES;
  int faceCount = 0;
  std::vector<int> emotionBoxes;
  std::string emotionsList[7] = {"angry", "disgust", "fear", "happy", "sad", "surprise", "neutral"};
  GstBuffer *imagesBuffer = gst_buffer_new();
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