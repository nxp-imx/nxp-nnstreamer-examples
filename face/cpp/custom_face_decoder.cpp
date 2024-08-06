/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_face_decoder.hpp"

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


void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
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
  boxesData->selectedBoxes.clear();

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
        boxesData->selectedBoxes.push_back(
            static_cast<int>(
                bufferInfo.bufferFP32[i] * CAMERA_INPUT_WIDTH
            )
        );
      }
      if ((col == 3) || (col == 5)) {
        boxesData->selectedBoxes.push_back(
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
  // clip to 16x16 min (imxvideoconvert constraint) and transform rectangular 
  // to square box using greater length
  for (int faceIndex = 0; faceIndex < 4 * countFaces; faceIndex += 4) {
    int w, h, cx, cy, d2;
    float k = 0.8; // scaling factor
    float d;
    float minwh = 16;
    w = boxesData->selectedBoxes.at(2 + faceIndex)
        - boxesData->selectedBoxes.at(0 + faceIndex) + 1;
    h = boxesData->selectedBoxes.at(3 + faceIndex)
        - boxesData->selectedBoxes.at(1 + faceIndex) + 1;
    cx = static_cast<int>(
        (boxesData->selectedBoxes.at(0 + faceIndex)
        + boxesData->selectedBoxes.at(2 + faceIndex))/2
    );
    cy = static_cast<int>(
      (boxesData->selectedBoxes.at(1 + faceIndex)
      + boxesData->selectedBoxes.at(3 + faceIndex))/2
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
    boxesData->selectedBoxes.at(0 + faceIndex) = cx - d2;
    boxesData->selectedBoxes.at(1 + faceIndex) = cy - d2;
    boxesData->selectedBoxes.at(2 + faceIndex) = cx + d2;
    boxesData->selectedBoxes.at(3 + faceIndex) = cy + d2;
  }
}


void drawCallback(GstElement* overlay,
                  cairo_t* cr,
                  guint64 timestamp,
                  guint64 duration,
                  gpointer user_data)
{
  DecoderData* boxesDataBuffer = (DecoderData *) user_data;
  DecoderData boxesData = *boxesDataBuffer;

  int numFaces = boxesData.selectedBoxes.size()/4;
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
    w = boxesData.selectedBoxes.at(2 + faceIndex)
        - boxesData.selectedBoxes.at(0 + faceIndex);
    h = boxesData.selectedBoxes.at(3 + faceIndex)
        - boxesData.selectedBoxes.at(1 + faceIndex);
    cairo_rectangle(cr,
                    boxesData.selectedBoxes.at(0 + faceIndex),
                    boxesData.selectedBoxes.at(1 + faceIndex),
                    w,
                    h);
  }

  cairo_stroke(cr);
}
