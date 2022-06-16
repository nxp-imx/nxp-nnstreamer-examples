#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022 NXP

function gst_exec_detection {

  # accelerated video scaling before inferencing
  local VIDEO_SCALE=$(video_scale_rgb_str ${MODEL_WIDTH} ${MODEL_HEIGHT})
  # accelerated video composition
  local VIDEO_MIXER=$(video_mixer_str "mix" "sink_0::zorder=2 sink_1::zorder=1" "0" "0.3" "${MODEL_LATENCY}")

  gst-launch-1.0 \
    v4l2src name=cam_src device=${CAMERA_DEVICE} num-buffers=-1 ! \
      video/x-raw,width=${CAMERA_WIDTH},height=${CAMERA_HEIGHT},framerate=${CAMERA_FPS}/1 ! \
      tee name=t \
    t. ! queue name=thread-nn max-size-buffers=2 leaky=2 ! \
      ${VIDEO_SCALE} \
      tensor_converter ! \
      ${TENSOR_PREPROCESS} \
      ${TENSOR_FILTER} \
      ${TENSOR_DECODER} \
      videoconvert ! \
      mix. \
    t. ! queue name=thread-img max-size-buffers=2 leaky=2 ! \
      videoconvert ! \
      ${VIDEO_MIXER} \
      autovideosink
}
