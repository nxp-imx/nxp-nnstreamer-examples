/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_emotion_decoder.hpp"

#include <math.h>
#include <cassert>
#include <iostream>

BufferInfo getTensorInfo(GstBuffer *buffer, int tensorIndex)
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


void checkNumTensor(GstBuffer *buffer, int numTensor)
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


void newDataCallback(GstElement *element,
                     GstBuffer *buffer,
                     gpointer user_data)
{
  DecoderData* boxesData = (DecoderData *) user_data;

  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);
  assert(boxesData->bufferSize == bufferInfo.size);

  std::vector<int> boxes;
  int faceCount = 0;
  for (int i = 0; ((i < MODEL_UFACE_NUMBER_BOXES)
                   && (faceCount < MODEL_UFACE_NUMBER_MAX)); i+= NUM_BOX_DATA) {
    // Keep only boxes with a score above the threshold
    if (bufferInfo.bufferFP32[i+1] > MODEL_UFACE_CLASSIFICATION_THRESHOLD) {
      faceCount += 1;
      // Store x1
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+2] * boxesData->camWidth)
        );
      // Store y1
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+3] * boxesData->camHeight)
        );
      // Store x2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+4] * boxesData->camWidth)
        );
      // Store y2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+5] * boxesData->camHeight)
        );
    }
  }

  // Transform rectangular to square boxe
  int w, h, cx, cy, d2;
  float k = 0.8; // scaling factor
  float minwh = 16; // minimum (imxvideoconvert constraint)
  float d;
  for (int faceIndex = 0; faceIndex < 4 * faceCount; faceIndex += 4) {
    w = boxes.at(2 + faceIndex) - boxes.at(0 + faceIndex) + 1;
    h = boxes.at(3 + faceIndex) - boxes.at(1 + faceIndex) + 1;
    cx = static_cast<int>((boxes.at(0 + faceIndex) + boxes.at(2 + faceIndex))/2);
    cy = static_cast<int>((boxes.at(1 + faceIndex) + boxes.at(3 + faceIndex))/2);

    d = std::max(w, h) * k;
    d = std::min(d, static_cast<float>(
        std::min(boxesData->camWidth, boxesData->camHeight)
    ));
    d = std::max(d, minwh);
    d2 = static_cast<int>(d/2);

    if ((cx + d2) >= boxesData->camWidth)
      cx = boxesData->camWidth - d2 - 1;
    if ((cx - d2) < 0)
      cx = d2;
    if ((cy + d2) >= boxesData->camHeight)
      cy = boxesData->camHeight - d2 - 1;
    if ((cy - d2) < 0)
      cy = d2;
    boxes.at(0 + faceIndex) = cx - d2;
    boxes.at(1 + faceIndex) = cy - d2;
    boxes.at(2 + faceIndex) = cx + d2;
    boxes.at(3 + faceIndex) = cy + d2;
  }

  boxesData->faceBoxes = boxes;
}


void pushBuffer(GstBuffer *buffer,
                std::vector<int> boxes,
                int index,
                DecoderData *boxesData)
{
  int faceIndex = index * 4;
  gint ctop = boxes.at(1 + faceIndex);
  gint cbottom = boxesData->camHeight - boxes.at(3 + faceIndex);
  gint cleft = boxes.at(0 + faceIndex);
  gint cright = boxesData->camWidth - boxes.at(2 + faceIndex);

  g_object_set((boxesData->videocrop), "top", ctop, NULL);
  g_object_set((boxesData->videocrop), "bottom", cbottom, NULL);
  g_object_set((boxesData->videocrop), "left", cleft, NULL);
  g_object_set((boxesData->videocrop), "right", cright, NULL);

  GstFlowReturn ret;
  g_signal_emit_by_name(boxesData->appSrc, "push-buffer", buffer, &ret);
  if (ret != GST_FLOW_OK) {
    log_error("Could not push buffer to appsrc\n");
    exit(-1);
  }
}


void getEmotionResult(GstBuffer *buffer,
                      std::vector<int> boxes,
                      int index,
                      DecoderData *boxesData)
{
  if (boxes.empty() || (buffer == nullptr)) {
    boxesData->result.emotions.clear();
    boxesData->result.values.clear();
    boxesData->result.boxes.clear();
    return;
  }

  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);

  float value = 0.0f;
  std::string emotion;
  // Get emotion and its associated probability for a detected face
  for (int i = 0; i < bufferInfo.size; i++) {
    if (value < bufferInfo.bufferFP32[i]) {
      value = bufferInfo.bufferFP32[i];
      emotion = boxesData->emotionsList[i];
    }
  }

  if (index == 0) {
    boxesData->result.emotions.clear();
    boxesData->result.values.clear();
    boxesData->result.boxes.clear();
  }

  int faceIndex = index * 4;
  boxesData->result.emotions.push_back(emotion);
  boxesData->result.values.push_back(value);
  boxesData->result.boxes.push_back(boxes.at(0 + faceIndex));
  boxesData->result.boxes.push_back(boxes.at(1 + faceIndex));
  boxesData->result.boxes.push_back(boxes.at(2 + faceIndex));
  boxesData->result.boxes.push_back(boxes.at(3 + faceIndex));
}


void secondaryNewDataCallback(GstElement* element,
                              GstBuffer* buffer,
                              gpointer user_data)
{
  DecoderData* boxesData = (DecoderData *) user_data;

  getEmotionResult(buffer, boxesData->emotionBoxes, boxesData->SubFaceCount, boxesData);
  boxesData->SubFaceCount += 1;
  int total = boxesData->emotionBoxes.size()/4;
  if (boxesData->SubFaceCount < total) {
    pushBuffer(boxesData->imagesBuffer, boxesData->emotionBoxes, boxesData->SubFaceCount, boxesData);
  } else {
    gst_buffer_unref(boxesData->imagesBuffer);
    boxesData->emotionBoxes.clear();
    boxesData->SubFaceCount = 0;
    boxesData->subActive = false;
  }
}


GstFlowReturn sinkCallback(GstAppSink* appsink, gpointer user_data)
{
  DecoderData *boxesData = (DecoderData *) user_data;

  GstSample *sample;
  g_signal_emit_by_name(appsink, "pull-sample", &sample);

  if (!sample) {
    log_error("Could not retrieves sample\n");
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  GstBuffer *buffer = gst_sample_get_buffer(sample);

  if (boxesData->subActive == true) {
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  boxesData->emotionBoxes = boxesData->faceBoxes;
  if (boxesData->emotionBoxes.size() == 0) {
    boxesData->result.emotions.clear();
    boxesData->result.values.clear();
    boxesData->result.boxes.clear();
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  boxesData->subActive = true;
  boxesData->SubFaceCount = 0;
  boxesData->imagesBuffer = gst_buffer_copy_deep(buffer);
  pushBuffer(buffer, boxesData->emotionBoxes, 0, boxesData);

  gst_sample_unref(sample);

  return GST_FLOW_OK;
}


void drawCallback(GstElement* overlay,
                  cairo_t* cr,
                  guint64 timestamp,
                  guint64 duration,
                  gpointer user_data)
{
  DecoderData *boxesData = (DecoderData *) user_data;

  int numFaces = boxesData->result.boxes.size()/4;
  if (numFaces == 0)
    return;

  std::vector<int> boxes = boxesData->result.boxes;
  std::vector<std::string> emotion = boxesData->result.emotions;
  std::vector<float> value = boxesData->result.values;

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 1, 1, 0);

  int w, h;
  for (int faceIndex = 0; faceIndex < 4 * numFaces; faceIndex += 4) {
    w = boxes.at(2 + faceIndex) - boxes.at(0 + faceIndex);
    h = boxes.at(3 + faceIndex) - boxes.at(1 + faceIndex);
    cairo_rectangle(cr, boxes.at(0 + faceIndex), boxes.at(1 + faceIndex), w, h);
    cairo_move_to(cr, boxes.at(0 + faceIndex) + 5, boxes.at(1 + faceIndex) + 10);
    std::string text = emotion.at(faceIndex/4)
                       + "("
                       + std::to_string(value.at(faceIndex/4)).substr(0,4)
                       + ")";
    cairo_show_text(cr, text.c_str());
  }
  cairo_stroke(cr);
 }