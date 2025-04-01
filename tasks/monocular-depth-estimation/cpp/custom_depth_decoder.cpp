/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_depth_decoder.hpp"

#include <math.h>
#include <cassert>
#include <iostream>
#include <algorithm>

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

  float *min = std::min_element(bufferInfo.bufferFP32, bufferInfo.bufferFP32 + bufferInfo.size);
  float *max = std::max_element(bufferInfo.bufferFP32, bufferInfo.bufferFP32 + bufferInfo.size);

  if ((*max - *min) > MODEL_THRESHOLD) {
    for (int i = 0; i < bufferInfo.size; i++) {
      data->output[i] = (guchar)std::round(255*(bufferInfo.bufferFP32[i] - *min) / (*max - *min));
    }
  } else {
    std::fill_n(&data->output[0], bufferInfo.size, 0);
  }

  data->size = bufferInfo.size;
  pushBuffer(data);
}


void pushBuffer(DecoderData* data)
{
  static GstClockTime timestamp = 0;
  GstBuffer *buffer = gst_buffer_new_allocate(NULL, data->size, NULL);
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  memcpy((guchar *)map.data, data->output, gst_buffer_get_size(buffer));
  GST_BUFFER_PTS(buffer) = timestamp;
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 2);
  timestamp += GST_BUFFER_DURATION(buffer);

  GstFlowReturn ret;
  g_signal_emit_by_name(data->appSrc, "push-buffer", buffer, &ret);
  gst_buffer_unmap(buffer, &map);
  gst_buffer_unref(buffer);
  if (ret != GST_FLOW_OK) {
    log_error("Could not push buffer to appsrc\n");
    exit(-1);
  }
}