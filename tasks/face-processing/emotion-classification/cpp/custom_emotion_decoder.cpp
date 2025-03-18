/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_emotion_decoder.hpp"

#include <math.h>
#include <cassert>
#include <iostream>
#include <sys/time.h>


// Font size of 15 pixels for an image width of 640 is default
const float fontFactor = 15.0f/640;
// Coordinates of the text to display the number of faces detected
// is (480, 18) for a image width of 640
const float xText = 480.0f/640;
const float yText = 18.0f/640;


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
            static_cast<int>(bufferInfo.bufferFP32[i+2] * boxesData->width)
        );
      // Store y1
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+3] * boxesData->height)
        );
      // Store x2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+4] * boxesData->width)
        );
      // Store y2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+5] * boxesData->height)
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
        std::min(boxesData->width, boxesData->height)
    ));
    d = std::max(d, minwh);
    d2 = static_cast<int>(d/2);

    if ((cx + d2) >= boxesData->width)
      cx = boxesData->width - d2 - 1;
    if ((cx - d2) < 0)
      cx = d2;
    if ((cy + d2) >= boxesData->height)
      cy = boxesData->height - d2 - 1;
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
  gint cbottom = boxesData->height - boxes.at(3 + faceIndex);
  gint cleft = boxes.at(0 + faceIndex);
  gint cright = boxesData->width - boxes.at(2 + faceIndex);

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
    boxesData->results.clear();
    return;
  }

  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);

  EmotionData data;
  data.confidence = 0.0f;
  // Get emotion and its associated probability for a detected face
  for (int i = 0; i < bufferInfo.size; i++) {
    if (data.confidence < bufferInfo.bufferFP32[i]) {
      data.confidence = bufferInfo.bufferFP32[i];
      data.emotion = boxesData->emotionsList[i];
    }
  }

  if (index == 0) {
    boxesData->results.clear();
  }

  data.box[0] = boxes.at(0 + index * 4);
  data.box[1] = boxes.at(1 + index * 4);
  data.box[2] = boxes.at(2 + index * 4);
  data.box[3] = boxes.at(3 + index * 4);
  boxesData->results.push_back(data);

  if (index + 1 == boxes.size()/4)
    boxesData->detections = boxesData->results;
}


void secondaryNewDataCallback(GstElement* element,
                              GstBuffer* buffer,
                              gpointer user_data)
{
  DecoderData* boxesData = (DecoderData *) user_data;

  getEmotionResult(buffer, boxesData->emotionBoxes, boxesData->emotionCount, boxesData);
  boxesData->emotionCount += 1;
  int total = boxesData->emotionBoxes.size()/4;
  if (boxesData->emotionCount < total) {
    pushBuffer(boxesData->imagesBuffer, boxesData->emotionBoxes, boxesData->emotionCount, boxesData);
  } else {
    boxesData->emotionBoxes.clear();
    boxesData->emotionCount = 0;
    boxesData->processEmotions = false;
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

  if (boxesData->processEmotions == true) {
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }
  
  if (boxesData->faceBoxes.empty()) {
    boxesData->results.clear();
    boxesData->detections.clear();
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  gst_buffer_unref(boxesData->imagesBuffer);
  boxesData->emotionBoxes = boxesData->faceBoxes;
  boxesData->processEmotions = true;
  boxesData->emotionCount = 0;
  GstBuffer *buffer = gst_sample_get_buffer(sample);
  boxesData->imagesBuffer = gst_buffer_copy_deep(buffer);
  gst_sample_unref(sample);

  pushBuffer(boxesData->imagesBuffer, boxesData->emotionBoxes, 0, boxesData);
  return GST_FLOW_OK;
}


void drawCallback(GstElement* overlay,
                  cairo_t* cr,
                  guint64 timestamp,
                  guint64 duration,
                  gpointer user_data)
{
  DecoderData *boxesData = (DecoderData *) user_data;
  std::vector<EmotionData> results = boxesData->detections;

  cairo_set_source_rgb(cr, 0.85, 0, 1);
  cairo_move_to(cr, boxesData->width * xText, boxesData->width * yText);
  cairo_select_font_face(cr,
                         "Arial",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, boxesData->width * fontFactor);
  cairo_show_text(cr, ("Faces detected: " + std::to_string(results.size())).c_str());

  if (boxesData->detections.empty())
    return;

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 1, 1, 0);

  int w, h;
  for (int i = 0; i < results.size(); i += 1) {
    int box[4] = {results.at(i).box[0], results.at(i).box[1], results.at(i).box[2], results.at(i).box[3]};
    w = box[2] - box[0];
    h = box[3] - box[1];
    cairo_rectangle(cr, box[0], box[1], w, h);
    cairo_move_to(cr, box[0], box[1] + h + 20);
    std::string text = results.at(i).emotion
                       + "("
                       + std::to_string(results.at(i).confidence).substr(0,4)
                       + ")";
    cairo_show_text(cr, text.c_str());
  }
  cairo_stroke(cr);
 }