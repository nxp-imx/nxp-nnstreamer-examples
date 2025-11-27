#!/bin/bash
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

function gst_exec_segmentation {

  # use a common format decoder output to a compositor for consistent alpha blending
  local VIDEO_COMPOSITOR_FORMAT="YUY2"
  # accelerated video scaling before inferencing
  local VIDEO_SCALE=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "" "1/1")
  # accelerated video composition
  local VIDEO_MIXER=$(accelerated_video_mixer_str "mix" "sink_0::zorder=1 sink_1::zorder=2" "1" "${ALPHA_VALUE:-0.5}" "${MODEL_LATENCY}")

  gst-launch-1.0 \
    multifilesrc location="${IMAGES_DIR}/image%04d.jpg" loop=true caps=image/jpeg,framerate=1/2 ! \
      jpegdec ! \
      ${VIDEO_SCALE} \
      tee   name=t \
    t. ! queue name=thread-nn max-size-buffers=2 leaky=2 ! \
      videoconvert ! video/x-raw,format=RGB ! \
      tensor_converter ! \
      ${TENSOR_PREPROCESS} \
      ${TENSOR_FILTER} \
      ${TENSOR_DECODER} \
      videoconvert ! video/x-raw,format=${VIDEO_COMPOSITOR_FORMAT} ! mix. \
    t. ! queue name=thread-img max-size-buffers=2 ! \
      ${VIDEO_MIXER} \
      waylandsink
}