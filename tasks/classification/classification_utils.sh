#!/bin/bash

# Copyright 2022, 2024-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

function gst_exec_classification {

  # accelerated video scaling before inferencing
  local VIDEO_SCALE=$(accelerated_video_scale_rgb_str ${MODEL_WIDTH} ${MODEL_HEIGHT})

  set -x

  gst-launch-1.0 \
    ${CAMERA_SOURCE} \
      video/x-raw,width=${CAMERA_WIDTH},height=${CAMERA_HEIGHT},framerate=${CAMERA_FPS}/1 !   \
      tee name=t \
    t. ! queue name=thread-nn max-size-buffers=2 leaky=2 ! \
      ${VIDEO_SCALE} \
      tensor_converter ! \
      ${TENSOR_PREPROCESS} \
      ${TENSOR_FILTER} \
      ${TENSOR_DECODER} \
      overlay.text_sink \
    t. ! queue name=thread-img max-size-buffers=2 leaky=2 ! \
      textoverlay name=overlay font-desc=\"Sans, 24\" ! \
      waylandsink sync=false
}
