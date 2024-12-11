/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_face_and_pose_decoder.hpp"

#include <math.h>
#include <cassert>

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


void newDataFaceCallback(GstElement* element,
                         GstBuffer* buffer,
                         gpointer user_data)
{
  FaceData* boxesData = (FaceData *) user_data;

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
            static_cast<int>(bufferInfo.bufferFP32[i+2] * INPUT_WIDTH)
        );
      // Store y1
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+3] * INPUT_HEIGHT)
        );
      // Store x2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+4] * INPUT_WIDTH)
        );
      // Store y2
      boxes.push_back(
            static_cast<int>(bufferInfo.bufferFP32[i+5] * INPUT_HEIGHT)
        );
    }
  }

  // Transform rectangular to square box
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
        std::min(INPUT_WIDTH, INPUT_HEIGHT)
    ));
    d = std::max(d, minwh);
    d2 = static_cast<int>(d/2);

    if ((cx + d2) >= INPUT_WIDTH)
      cx = INPUT_WIDTH - d2 - 1;
    if ((cx - d2) < 0)
      cx = d2;
    if ((cy + d2) >= INPUT_HEIGHT)
      cy = INPUT_HEIGHT - d2 - 1;
    if ((cy - d2) < 0)
      cy = d2;
    boxes.at(0 + faceIndex) = cx - d2;
    boxes.at(1 + faceIndex) = cy - d2;
    boxes.at(2 + faceIndex) = cx + d2;
    boxes.at(3 + faceIndex) = cy + d2;
  }

  boxesData->faceCount = faceCount;
  boxesData->selectedBoxes = boxes;
}


void drawFaceCallback(GstElement* overlay,
                      cairo_t* cr,
                      guint64 timestamp,
                      guint64 duration,
                      gpointer user_data)
{
  FaceData* boxesData = (FaceData *) user_data;

  int numFaces = boxesData->faceCount;
  std::vector<int> boxes = boxesData->selectedBoxes;

  cairo_set_source_rgb(cr, 0.85, 0, 1);
  cairo_move_to(cr, 480, 18);
  cairo_select_font_face(cr,
                         "Arial",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, 15);
  cairo_show_text(cr, ("Faces detected: " + std::to_string(numFaces)).c_str());

  cairo_set_source_rgb(cr, 1, 0, 0);
  cairo_set_line_width(cr, 1.0);

  int w, h;
  for (int faceIndex = 0; faceIndex < 4 * numFaces; faceIndex += 4) {
    w = boxes.at(2 + faceIndex) - boxes.at(0 + faceIndex);
    h = boxes.at(3 + faceIndex) - boxes.at(1 + faceIndex);
    cairo_rectangle(cr, boxes.at(0 + faceIndex), boxes.at(1 + faceIndex), w, h);
  }
  cairo_stroke(cr);
}


void newDataPoseCallback(GstElement* element,
                         GstBuffer* buffer,
                         gpointer user_data)
{
  PoseData* kptsData = (PoseData *) user_data;
  
  BufferInfo bufferInfo;
  checkNumTensor(buffer, 1);
  bufferInfo = getTensorInfo(buffer, 0);

  int row = 0;
  int col = 0;
  float score = 0;
  float valid = 0;
  for (int i = 0; i < bufferInfo.size; i++) {

    if ((col == X_INDEX) or (col == Y_INDEX)) {
      kptsData->npKpts[row][col] = bufferInfo.bufferFP32[i] * 480;
    } else {
      kptsData->npKpts[row][col] = bufferInfo.bufferFP32[i];
      score = kptsData->npKpts[row][SCORE_INDEX];
      valid = (score >= SCORE_THRESHOLD);
      kptsData->npKpts[row][col] = valid;
    }

    col+=1;
    if (col == 3)
      row += 1;
    col = col % 3;
  }
}


void drawPoseCallback(GstElement* overlay,
                      cairo_t* cr,
                      guint64 timestamp,
                      guint64 duration,
                      gpointer user_data)
{
  PoseData* kptsData = (PoseData *) user_data;
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

  for(int i = 0; i < KPT_SIZE; i++) {
    npKpt = kptsData->npKpts[i];
    valid = npKpt[SCORE_INDEX];
    if (valid != 1.0)
      continue;

    xKpt = npKpt[X_INDEX];
    yKpt = npKpt[Y_INDEX];

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
      valid = npConnect[SCORE_INDEX];
      if (valid != 1.0)
        continue;
      xConnect = npConnect[X_INDEX];
      yConnect = npConnect[Y_INDEX];
      cairo_move_to(cr, xKpt, yKpt);
      cairo_line_to(cr, xConnect, yConnect);
    }
    cairo_stroke(cr);
  }
}