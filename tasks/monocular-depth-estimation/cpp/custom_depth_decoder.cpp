/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_depth_decoder.hpp"

#include <math.h>
#include <cassert>
#include <iostream>
#include <algorithm>
#ifdef _OPENMP
#include <omp.h>
#endif

BufferInfo getTensorInfo(GstBuffer* buffer, int tensorIndex)
{
  BufferInfo bufferInfo;
  GstMapInfo info;
  GstMemory* memKpts = gst_buffer_peek_memory(buffer, tensorIndex);
  bool result = gst_memory_map(memKpts, &info, GST_MAP_READ);
  if (result) {
    bufferInfo.bufferFP32 = reinterpret_cast<float*>(info.data);
    bufferInfo.size = info.size/sizeof(float);
    gst_memory_unmap(memKpts, &info);
  } else {
    gst_memory_unmap(memKpts, &info);
    log_error("Can't access buffer in memory\n");
    exit(-1);
  }
  return bufferInfo;
}


void checkNumTensor(GstBuffer* buffer, int numTensor)
{
  if (!GST_IS_BUFFER (buffer)) {
    log_error("Received invalid buffer\n");
    exit(-1);
  }
  uint mem_blocks = gst_buffer_n_memory(buffer);
  if (mem_blocks != numTensor) {
    log_error("Number of tensors invalid : %d\n", mem_blocks);
    exit(-1);
  }
}


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data)
{
  DecoderData* data = (DecoderData *) user_data;

  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);
  float* outputData = bufferInfo.bufferFP32;

  if (!outputData || bufferInfo.size != MODEL_OUTPUT_DIM) {
    log_error("Invalid model output data or size mismatch\n");
    exit(-1);
  }

  // Parallel min/max reduction using OpenMP
  float minVal = outputData[0];
  float maxVal = outputData[0];

#ifdef _OPENMP
  #pragma omp parallel for reduction(min:minVal) reduction(max:maxVal) schedule(static)
#endif
  for (int i = 1; i < MODEL_OUTPUT_DIM; i++) {
    const float val = outputData[i];
    if (val < minVal) minVal = val;
    if (val > maxVal) maxVal = val;
  }

  const float range = maxVal - minVal;
  if (range > MODEL_THRESHOLD) {
    const float scale = 255.0f / range;
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < MODEL_OUTPUT_DIM; i++) {
      data->output[i] = (guchar)std::round(scale * (outputData[i] - minVal));
    }
  } else {
    memset(data->output, 0, MODEL_OUTPUT_DIM);
  }

  pushBuffer(data);
}


void pushBuffer(DecoderData* data)
{
  // Create buffer that wraps our existing data (zero-copy)
  GstBuffer *buffer = gst_buffer_new_wrapped_full(
    GST_MEMORY_FLAG_READONLY,
    data->output,
    MODEL_OUTPUT_DIM,
    0,
    MODEL_OUTPUT_DIM,
    NULL,
    NULL
  );

  GstFlowReturn ret;
  g_signal_emit_by_name(data->appSrc, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);

  if (ret != GST_FLOW_OK) {
    log_error("Could not push buffer to appsrc\n");
    exit(-1);
  }
}