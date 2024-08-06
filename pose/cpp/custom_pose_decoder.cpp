/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "custom_pose_decoder.hpp"

#include <math.h>

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
    log_error("Number of tensors invalid\n");
    exit(-1);
  }
}


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data)
{
  DecoderData* kptsData = (DecoderData *) user_data;
  
  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);

  int row = 0;
  int col = 0;
  float score = 0;
  float valid = 0;
  for (int i = 0; i < bufferInfo.size; i++) {

    if ((col == kptsData->xIndex) or (col == kptsData->yIndex)) {
      kptsData->npKpts[row][col] = bufferInfo.bufferFP32[i] * 480;
    } else {
      kptsData->npKpts[row][col] = bufferInfo.bufferFP32[i];
      score = kptsData->npKpts[row][kptsData->scoreIndex];
      valid = (score >= kptsData->scoreThreshold);
      kptsData->npKpts[row][col] = valid;
    }

    col+=1;
    if (col == 3)
      row += 1;
    col = col % 3;
  }
}


void drawCallback(GstElement* overlay,
                  cairo_t* cr,
                  guint64 timestamp,
                  guint64 duration,
                  gpointer user_data)
{
  DecoderData* kptsData = (DecoderData *) user_data;
  float valid;
  float* npKpt;
  float xKpt;
  float yKpt;
  int* connections;
  float* npConnect;
  float xConnect;
  float yConnect;
  cairo_select_font_face(cr,
                         "Arial",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_line_width(cr, 1.0);

  for(int i = 0; i < kptsData->kptSize; i++) {
    npKpt = kptsData->npKpts[i];
    valid = npKpt[kptsData->scoreIndex];
    if ((valid != 1.0))
      continue;

    xKpt = npKpt[kptsData->xIndex];
    yKpt = npKpt[kptsData->yIndex];

    // Draw keypoint spot
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_arc(cr, xKpt, yKpt, 1, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_stroke(cr);
    
    // Draw keypoint label
    cairo_set_source_rgb(cr, 0, 1, 1);
    cairo_set_font_size(cr, 10.0);
    cairo_move_to(cr, xKpt + 5, yKpt + 5);
    cairo_show_text(cr, kptsData->kptLabels[i].c_str());

    // Draw keypoint connections
    cairo_set_source_rgb(cr, 0, 1, 0);
    connections = kptsData->kptConnect[i];
    for(int j = 0; j < 3; j++) {
      if (connections[j] == -1)
        break;
      npConnect = kptsData->npKpts[connections[j]];
      valid = npConnect[kptsData->scoreIndex];
      if (valid != 1.0)
        continue;
      xConnect = npConnect[kptsData->xIndex];
      yConnect = npConnect[kptsData->yIndex];
      cairo_move_to(cr, xKpt, yKpt);
      cairo_line_to(cr, xConnect, yConnect);
    }
    cairo_stroke(cr);
  }
}
