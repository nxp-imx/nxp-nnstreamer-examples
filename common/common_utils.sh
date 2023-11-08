#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022-2023 NXP

function error {
   local message="$1"
   if [ ! -z "${message}" ]; then
       echo "${message}"
   fi
   exit 1
}

# setup environment
function setup_env {
  # detect i.MX in use
  local UNAME=$(uname -a)
  if [[ "${UNAME}" =~ "imx8mp" ]]; then
    IMX="IMX8MP"
    # Store on disk .nb files that contains the result of the OpenVX graph compilation
    # This feature is only available for iMX8MPlus to get the warmup time only once
    export VIV_VX_ENABLE_CACHE_GRAPH_BINARY="1"
    export VIV_VX_CACHE_BINARY_GRAPH_DIR=$HOME
  fi
  if [[ "${UNAME}" =~ "imx93" ]]; then
    IMX="IMX93"
  fi
  if [ -z ${IMX} ]; then
    error "platform not supported"
  fi
  if [ "${IMX}" = "IMX93" ] && [ "${BACKEND}" = "GPU" ]; then
    error "Invalid combination ${IMX}/${BACKEND}"
  fi

  REQUIRED_CAMERA=${REQUIRED_CAMERA:-1}
  if [ "${REQUIRED_CAMERA}" -gt 0 ] ; then
    # default camera configuration (can also be overriden by user)
    declare -A CAMERA_DEVICE_DEFAULT
    CAMERA_DEVICE_DEFAULT[IMX8MP]="/dev/video3"
    CAMERA_DEVICE_DEFAULT[IMX93]="/dev/video0"
    local CAMERA_DEVICE_DEFAULT_IMX=${CAMERA_DEVICE_DEFAULT[${IMX}]}

    CAMERA_DEVICE="${CAMERA_DEVICE:-${CAMERA_DEVICE_DEFAULT_IMX}}"
    if [ ! -c ${CAMERA_DEVICE} ]; then
      local MSG="Camera device ${CAMERA_DEVICE} not found."
      MSG="$MSG Check device and set CAMERA_DEVICE variable appropriately."
      error "$MSG"
    fi

    CAMERA_WIDTH="${CAMERA_WIDTH:-640}"
    CAMERA_HEIGHT="${CAMERA_HEIGHT:-480}"
    CAMERA_FPS="${CAMERA_FPS:-30}"
  fi
 
  # backend default configuration
  BACKEND="${BACKEND:-NPU}"
  case "${BACKEND}" in
  CPU|NPU)
    export USE_GPU_INFERENCE=0 ;;
  GPU)
    export USE_GPU_INFERENCE=1 ;;
  *)
    error "invalid backend ${BACKEND}" ;;
  esac

  # GPU2D API
  case "${IMX}" in
  IMX8MP)
    GPU2D_API="G2D" ;;
  IMX93)
    GPU2D_API="PXP" ;;
  *)
    GPU2D_API="NONE" ;;
  esac

  # XXX: i.MX93: mute noisy ethos-u kernel driver
  if [ "${IMX}" = "IMX93" ]; then
    echo 4 > /proc/sys/kernel/printk
  fi
}

# accelerated scaling/conversion
# $1 video output width
# $2 video output height
function video_scale_rgb_str {
  local VIDEO_SCALE
  local OUTPUT_WIDTH=$1
  local OUTPUT_HEIGHT=$2

  case "${GPU2D_API}" in
  G2D)
    # g2d-based
    VIDEO_SCALE="\
      imxvideoconvert_g2d ! \
      video/x-raw,width=${OUTPUT_WIDTH},height=${OUTPUT_HEIGHT},format=RGBA ! \
      videoconvert ! video/x-raw,format=RGB !\
    "
    ;;
  PXP)
    # pxp-based
    VIDEO_SCALE="\
      imxvideoconvert_pxp ! \
      video/x-raw,width=${OUTPUT_WIDTH},height=${OUTPUT_HEIGHT},format=BGR ! \
      videoconvert ! video/x-raw,format=RGB !\
    "
    ;;
  *)
    # cpu-based
    VIDEO_SCALE="\
      videoscale ! videoconvert ! \
      video/x-raw,width=${OUTPUT_WIDTH},height=${OUTPUT_HEIGHT},format=RGB ! \
    "
    ;;
  esac

  echo "${VIDEO_SCALE}"
}


# accelerated video mixer
# $1 mixer name
# $2 mixer extra parameters
# parameters for compositor not supporting alpha channel (pxp)
# $3 alpha channel pad number
# $4 alpha channel value
# $5 max inputs latency in nanoseconds
function video_mixer_str {
  local VIDEO_MIX
  local MIXER_NAME=$1
  local MIXER_ARGS=$2
  local ALPHA_CHANNEL_PAD=$3
  local ALPHA_CHANNEL_VALUE=$4
  local LATENCY=$5

  case "${GPU2D_API}" in
  G2D)
    # g2d-based
    VIDEO_MIXER="imxcompositor_g2d name=${MIXER_NAME} ${MIXER_ARGS}"
    ;;
  PXP)
    # pxp-based
    VIDEO_MIXER="\
      imxcompositor_pxp name=${MIXER_NAME} ${MIXER_ARGS} \
      sink_${ALPHA_CHANNEL_PAD}::alpha=${ALPHA_CHANNEL_VALUE} \
    "
    ;;
  *)
    # cpu-based
    VIDEO_MIXER="videomixer name=${MIXER_NAME} ${MIXER_ARGS}"
    ;;
  esac

  # configure model latency
  case "${GPU2D_API}" in
  G2D|PXP)
    if [ -n "${LATENCY}" ] && [ "${LATENCY}" -ne 0 ]; then
      VIDEO_MIXER="${VIDEO_MIXER} latency=${LATENCY} min-upstream-latency=${LATENCY} !"
    else
      VIDEO_MIXER="${VIDEO_MIXER} !"
    fi
    ;;
  *)
    VIDEO_MIXER="${VIDEO_MIXER} !"
    ;;
  esac

  echo "${VIDEO_MIXER}"
}
