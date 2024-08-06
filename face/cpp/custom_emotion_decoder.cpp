/**
 * Copyright 2024 NXP
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

  int row = 0;
  int col = 0;
  int countFaces = 0;
  bool selected[MODEL_UFACE_NUMBER_NMS_BOXES] = { false };
  boxesData->faceBoxes.clear();

  // Get normalized boxes coordinates (x1,y1,x2,y2), and which boxes we keep
  for (int i = 0; (i < bufferInfo.size) 
                  && (countFaces <= MODEL_UFACE_NUMBER_MAX) ; i++) {
    if (col == 1) {
      if (bufferInfo.bufferFP32[i] > MODEL_UFACE_CLASSIFICATION_THRESHOLD) {
        countFaces += 1;
        selected[row] = true;
      }
    }

    // Get only boxes to display, and
    // rescale boxes per actual input video resolution
    if (selected[row] == true) {
      if ((col == 2) || (col == 4)) {
        boxesData->faceBoxes.push_back(
            static_cast<int>(
                bufferInfo.bufferFP32[i] * CAMERA_INPUT_WIDTH
            )
        );
      }
      if ((col == 3) || (col == 5)) {
        boxesData->faceBoxes.push_back(
            static_cast<int>(
                bufferInfo.bufferFP32[i] * CAMERA_INPUT_HEIGHT
            )
        );
      }
    }

    col += 1;
    if (col == NUMBER_OF_INFOS)
      row += 1;
    col %= NUMBER_OF_INFOS;
  }

  countFaces %= MODEL_UFACE_NUMBER_MAX;
  // clip to 16x16 min (imxvideoconvert constraint)
  for (int faceIndex = 0; faceIndex < 4 * countFaces; faceIndex += 4) {
  // Transform rectangular to square box using greater length
    int w, h, cx, cy, d2;
    float k = 0.8; // scaling factor
    float d;
    float minwh = 16;
    w = boxesData->faceBoxes.at(2 + faceIndex)
        - boxesData->faceBoxes.at(0 + faceIndex) + 1;
    h = boxesData->faceBoxes.at(3 + faceIndex)
        - boxesData->faceBoxes.at(1 + faceIndex) + 1;
    cx = static_cast<int>(
        (boxesData->faceBoxes.at(0 + faceIndex)
        + boxesData->faceBoxes.at(2 + faceIndex))/2
    );
    cy = static_cast<int>(
      (boxesData->faceBoxes.at(1 + faceIndex)
      + boxesData->faceBoxes.at(3 + faceIndex))/2
    );
    
    d = std::max(w, h) * k;
    d = std::min(d, static_cast<float>(
        std::min(CAMERA_INPUT_WIDTH, CAMERA_INPUT_HEIGHT)
    ));
    d = std::max(d, minwh);
    d2 = static_cast<int>(d/2);

    if ((cx + d2) >= CAMERA_INPUT_WIDTH)
      cx = CAMERA_INPUT_WIDTH - d2 - 1;
    if ((cx - d2) < 0)
      cx = d2;
    if ((cy + d2) >= CAMERA_INPUT_HEIGHT)
      cy = CAMERA_INPUT_HEIGHT - d2 - 1;
    if ((cy - d2) < 0)
      cy = d2;

    boxesData->faceBoxes.at(0 + faceIndex) = cx - d2;
    boxesData->faceBoxes.at(1 + faceIndex) = cy - d2;
    boxesData->faceBoxes.at(2 + faceIndex) = cx + d2;
    boxesData->faceBoxes.at(3 + faceIndex) = cy + d2;
  }
}


void pushBuffer(GstBuffer *buffer,
                std::vector<int> boxes,
                int index,
                DecoderData *boxesData)
{
  int faceIndex = index * 4;
  gint ctop = boxes.at(1 + faceIndex);
  gint cbottom = CAMERA_INPUT_HEIGHT - boxes.at(3 + faceIndex);
  gint cleft = boxes.at(0 + faceIndex);
  gint cright = CAMERA_INPUT_WIDTH - boxes.at(2 + faceIndex);

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


void getEmotionResult(GstBuffer* buffer,
                      std::vector<int> boxes,
                      int index,
                      DecoderData* boxesData)
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
  for (int i = 0; i < bufferInfo.size; i++) {
    if (value < bufferInfo.bufferFP32[i]) {
      value = bufferInfo.bufferFP32[i];
      emotion = boxesData->modelDeepfaceClasses[i];
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
  
  int count = boxesData->SubFaceCount;
  getEmotionResult(buffer, boxesData->emotionBoxes, count, boxesData);
  count += 1;
  int total = boxesData->emotionBoxes.size()/4;

  if (count < total) {
    GstBuffer *input = boxesData->imagesBuffer;
    boxesData->SubFaceCount = count;
    pushBuffer(input, boxesData->emotionBoxes, count, boxesData);
  } else {
    boxesData->imagesBuffer = nullptr;
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
    gst_sample_unref(sample);
    return GST_FLOW_OK;
  }

  boxesData->subActive = true;
  boxesData->SubFaceCount = 0;
  boxesData->imagesBuffer = buffer;
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
  std::vector<int> box = boxesData->result.boxes;
  std::vector<std::string> emotion = boxesData->result.emotions;
  std::vector<float> value = boxesData->result.values;

  int numFaces = box.size()/4;
  if (numFaces == 0)
    return;

  cairo_set_line_width(cr, 1.0);

  int w, h;
  for (int faceIndex = 0; faceIndex < 4 * numFaces; faceIndex += 4) {
    if (faceIndex == 0)
      cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_rectangle(cr, box.at(0 + faceIndex), box.at(1 + faceIndex), box.at(2 + faceIndex) - box.at(0 + faceIndex), box.at(3 + faceIndex) - box.at(1 + faceIndex));
    cairo_move_to(cr, box.at(0 + faceIndex), box.at(3 + faceIndex) + 20);
    if ((emotion.size() == 0) || (value.size() == 0))
      return;
    cairo_show_text(cr, (emotion.at(faceIndex/4) + "(" +
                        std::to_string(value.at(faceIndex/4)) + ")").c_str());
  }
  cairo_stroke(cr);
 }