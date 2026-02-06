/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "custom_face_decoder.hpp"

#include <math.h>
#include <cassert>
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


// ============================
// IoU between two boxes
// boxes vector uses [x1,y1,x2,y2] integer coords
// ============================
static float IoU(const int* a, const int* b)
{
    int x1 = std::max(a[0], b[0]);
    int y1 = std::max(a[1], b[1]);
    int x2 = std::min(a[2], b[2]);
    int y2 = std::min(a[3], b[3]);

    int inter_w = std::max(0, x2 - x1 + 1);
    int inter_h = std::max(0, y2 - y1 + 1);
    int inter   = inter_w * inter_h;

    int areaA = (a[2] - a[0] + 1) * (a[3] - a[1] + 1);
    int areaB = (b[2] - b[0] + 1) * (b[3] - b[1] + 1);

    return inter / (float)(areaA + areaB - inter);
}


// ============================
// Apply Non‑Max Suppression
// boxes = [x1,y1,x2,y2]...
// conf  = confidences per box
// iou_threshold = e.g. 0.35
// ============================
static void NMS(std::vector<int>& boxes,
                const std::vector<float>& confs,
                float iou_threshold)
{
    int count = confs.size();
    if (count == 0) return;

    // Sort indices by confidence descending
    std::vector<int> idx(count);
    for (int i = 0; i < count; i++)
        idx[i] = i;

    std::sort(idx.begin(), idx.end(),
              [&](int a, int b){ return confs[a] > confs[b]; });

    std::vector<char> removed(count, 0);

    for (int i = 0; i < count; i++)
    {
        if (removed[idx[i]]) continue;

        for (int j = i+1; j < count; j++)
        {
            if (removed[idx[j]]) continue;

            float iou = IoU(&boxes[idx[i]*4], &boxes[idx[j]*4]);
            if (iou > iou_threshold)
            {
                removed[idx[j]] = 1;
            }
        }
    }

    // Filter results into a new vector
    std::vector<int> out;
    for (int i = 0; i < count; i++)
    {
        if (!removed[i])
        {
            out.push_back(boxes[i*4+0]);
            out.push_back(boxes[i*4+1]);
            out.push_back(boxes[i*4+2]);
            out.push_back(boxes[i*4+3]);
        }
    }

    boxes.swap(out);
}


// ============================
// UPDATED newDataCallback()
// ============================
void newDataCallback(GstElement* element,
                     GstBuffer* buffer,
                     gpointer user_data)
{
    DecoderData* boxesData = (DecoderData*)user_data;

    checkNumTensor(buffer, 1);
    BufferInfo buf = getTensorInfo(buffer, 0);

    const int GH = 9;
    const int GW = 16;
    const int C  = 38;

    const float anchor_sizes[2] = {80.f, 30.f};
    const int num_anchors = 2;

    const int input_h = 144;
    const int input_w = 256;

    float* t = buf.bufferFP32;

    std::vector<int> boxes;
    std::vector<float> confs;
    int rawBoxCount = 0;

    auto compute_anchor_center_x = [&](int gx) {
        return (gx + 1) * (float)input_w / (GW + 1);
    };
    auto compute_anchor_center_y = [&](int gy) {
        return (gy + 1) * (float)input_h / (GH + 1);
    };

    // ===== Decode all boxes (without max-limit yet) =====
    for (int gy = 0; gy < GH; ++gy)
    {
        for (int gx = 0; gx < GW; ++gx)
        {
            int cell_offset = (gy * GW + gx) * C;

            for (int a = 0; a < num_anchors; ++a)
            {
                int conf_ch = 2 + a;
                float conf = t[cell_offset + conf_ch];
                if (conf < 0.45f)
                    continue;

                float anchor_w = anchor_sizes[a];
                float anchor_h = anchor_sizes[a];
                float anchor_x = compute_anchor_center_x(gx);
                float anchor_y = compute_anchor_center_y(gy);

                int base = cell_offset + (a == 0 ? 4 : 8);

                float dx = t[base + 0];
                float dy = t[base + 1];
                float dw = t[base + 2];
                float dh = t[base + 3];

                float box_cx = anchor_x + dx * anchor_w;
                float box_cy = anchor_y + dy * anchor_h;
                float box_w  = anchor_w * dw;
                float box_h  = anchor_h * dh;

                float x1 = box_cx - box_w * 0.5f;
                float y1 = box_cy - box_h * 0.5f;
                float x2 = box_cx + box_w * 0.5f;
                float y2 = box_cy + box_h * 0.5f;

                x1 *= ((float)boxesData->camWidth  / input_w);
                x2 *= ((float)boxesData->camWidth  / input_w);
                y1 *= ((float)boxesData->camHeight / input_h);
                y2 *= ((float)boxesData->camHeight / input_h);

                boxes.push_back((int)x1);
                boxes.push_back((int)y1);
                boxes.push_back((int)x2);
                boxes.push_back((int)y2);

                confs.push_back(conf);
                rawBoxCount++;
            }
        }
    }

    // ===== Apply NMS =====
    NMS(boxes, confs, 0.35f);

    int faceCount = boxes.size() / 4;

    // ===== Apply square-box logic to final NMS‑filtered boxes =====
    float k = 0.8f;
    float minwh = 16.f;

    for (int i = 0; i < boxes.size(); i += 4)
    {
        int w = boxes[i+2] - boxes[i] + 1;
        int h = boxes[i+3] - boxes[i+1] + 1;

        int cx = (boxes[i] + boxes[i+2]) / 2;
        int cy = (boxes[i+1] + boxes[i+3]) / 2;

        float d = std::max(w, h) * k;
        d = std::min(d, (float)std::min(boxesData->camWidth, boxesData->camHeight));
        d = std::max(d, minwh);

        int d2 = (int)(d / 2);

        if (cx + d2 >= boxesData->camWidth)  cx = boxesData->camWidth - d2 - 1;
        if (cx - d2 < 0)                     cx = d2;
        if (cy + d2 >= boxesData->camHeight) cy = boxesData->camHeight - d2 - 1;
        if (cy - d2 < 0)                     cy = d2;

        boxes[i]   = cx - d2;
        boxes[i+1] = cy - d2;
        boxes[i+2] = cx + d2;
        boxes[i+3] = cy + d2;
    }

    // Cap number of faces if needed
    if (faceCount > MODEL_UFACE_NUMBER_MAX)
    {
        boxes.resize(MODEL_UFACE_NUMBER_MAX * 4);
        faceCount = MODEL_UFACE_NUMBER_MAX;
    }

    boxesData->faceCount = faceCount;
    boxesData->selectedBoxes = boxes;
}


void drawCallback(GstElement* overlay,
                  cairo_t* cr,
                  guint64 timestamp,
                  guint64 duration,
                  gpointer user_data)
{
  DecoderData* boxesData = (DecoderData *) user_data;

  int numFaces = boxesData->faceCount;
  std::vector<int> boxes = boxesData->selectedBoxes;

  cairo_set_source_rgb(cr, 0.85, 0, 1);
  cairo_move_to(cr, boxesData->camWidth * (1 - 160.0/640), boxesData->camWidth * 18/640);
  cairo_select_font_face(cr,
                         "Arial",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, boxesData->camWidth * 15/640);
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
