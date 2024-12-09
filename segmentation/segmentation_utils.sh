#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2024 NXP

# segmentation pipeline g2d accelerated for video rescaling
# video compositor elements are not working for 513x513 - nnstreamer bug
# https://github.com/nnstreamer/nnstreamer/issues/3866
# fall back to regular videomixer

function gst_exec_segmentation {

  # accelerated video scaling before inferencing
  local VIDEO_SCALE=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} ${SCALE_FORMAT})

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
      videoconvert ! mix. \
    t. ! queue name=thread-img max-size-buffers=2 ! \
      videomixer name=mix sink_1::alpha=0.4 sink_0::alpha=1.0 background=3 ! \
      waylandsink
}