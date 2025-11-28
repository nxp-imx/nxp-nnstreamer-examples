/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef DEPTH_CUSTOM_DEPTH_DECODER_H_
#define DEPTH_CUSTOM_DEPTH_DECODER_H_

#include <string>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>
#include <vector>

#include "logging.hpp"

#define MODEL_THRESHOLD   1e-6
#define MODEL_OUTPUT_DIM  65536


typedef struct {
  guchar output[MODEL_OUTPUT_DIM];
  GstElement *appSrc;
} DecoderData;


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data);


void pushBuffer(DecoderData* data);


typedef struct {
  int size;
  float* bufferFP32;
} BufferInfo;


BufferInfo getTensorInfo(GstBuffer* buffer, int tensorIndex);


void checkNumTensor(GstBuffer* buffer, int numTensor);

#endif