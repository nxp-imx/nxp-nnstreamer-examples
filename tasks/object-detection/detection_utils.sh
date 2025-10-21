#!/bin/bash
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

function gst_exec_detection {

  # accelerated video scaling before inferencing
  local VIDEO_SCALE=$(accelerated_video_scale_rgb_str ${MODEL_WIDTH} ${MODEL_HEIGHT})
  # accelerated video composition
  # ALPHA_VALUE env var allows runtime customization for alpha blending (only for i.MX93 with pxp) */
  local VIDEO_MIXER=$(accelerated_video_mixer_str "mix" "sink_0::zorder=2 sink_1::zorder=1" "0" "${ALPHA_VALUE:-0.3}" "${MODEL_LATENCY}")

  set -x

  gst-launch-1.0 \
    ${CAMERA_SOURCE} \
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
      waylandsink
}
