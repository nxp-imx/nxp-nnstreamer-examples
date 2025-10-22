#!/bin/bash
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

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
  if [[ "${UNAME}" =~ "imx95" ]]; then
    IMX="IMX95"
    # Disable input tensor zero-copy feature not yet supported by NNStreamer
    # This feature was enabled by default for Neutron NPUs
    export NEUTRON_ENABLE_ZERO_COPY='0'
  fi
  if [ -z ${IMX} ]; then
    error "platform not supported"
  fi
  if [ "${IMX}" = "IMX93" ] && [ "${BACKEND}" = "GPU" ]; then
    error "Invalid combination ${IMX}/${BACKEND}"
  fi


# Create camera source pipeline segment
# This function allows selecting between v4l2src and libcamera based on platform
function camera_source_str {

  REQUIRED_CAMERA=${REQUIRED_CAMERA:-1}
  if [ "${REQUIRED_CAMERA}" -gt 0 ] ; then

    # Ensure IMX variable is set
    if [ -z "${IMX}" ]; then
      error "IMX variable not set"
    fi

    # Determine default camera type based on platform
    local CAMERA_BACKEND_DEFAULT
    case "${IMX}" in
    IMX95)
      # Future: libcamera will be default for IMX95
      CAMERA_BACKEND_DEFAULT="v4l2"
      ;;
    *)
      CAMERA_BACKEND_DEFAULT="v4l2"
      ;;
    esac

    # Use user-specified CAMERA_BACKEND or fall back to platform default
    CAMERA_BACKEND="${CAMERA_BACKEND:-${CAMERA_BACKEND_DEFAULT}}"

    # Validate camera type is supported on current platform
    case "${IMX}" in
    IMX95)
      # Both v4l2 and libcamera supported for i.MX95 platform
      case "${CAMERA_BACKEND}" in
      v4l2|libcamera)
        ;;
      *)
        error "invalid camera type ${CAMERA_BACKEND} for platform ${IMX}. Supported: v4l2, libcamera"
        ;;
      esac
      ;;
    *)
      # Default to v4l2 only for other platforms
      case "${CAMERA_BACKEND}" in
      v4l2)
        ;;
      *)
      error "invalid camera type ${CAMERA_BACKEND} for platform ${IMX}. Supported: v4l2"
        ;;
      esac
      ;;
    esac

    # Generate camera source pipeline based on resolved type
    case "${CAMERA_BACKEND}" in
    v4l2)

      # default camera configuration fo v4l2 source (can also be overridden by user)
      declare -A CAMERA_DEVICE_DEFAULT
      CAMERA_DEVICE_DEFAULT[IMX8MP]="/dev/video3"
      CAMERA_DEVICE_DEFAULT[IMX93]="/dev/video0"
      CAMERA_DEVICE_DEFAULT[IMX95]="/dev/video13"
      local CAMERA_DEVICE_DEFAULT_IMX=${CAMERA_DEVICE_DEFAULT[${IMX}]}

      CAMERA_DEVICE="${CAMERA_DEVICE:-${CAMERA_DEVICE_DEFAULT_IMX}}"
      if [ ! -c ${CAMERA_DEVICE} ]; then
        local MSG="Camera device ${CAMERA_DEVICE} not found."
        MSG="$MSG Check device with 'v4l2-ctl --list-devices' and set CAMERA_DEVICE variable appropriately."
        error "$MSG"
      fi

      CAMERA_WIDTH="${CAMERA_WIDTH:-640}"
      CAMERA_HEIGHT="${CAMERA_HEIGHT:-480}"
      CAMERA_FPS="${CAMERA_FPS:-30}"

      CAMERA_SOURCE="v4l2src name=cam_src device=${CAMERA_DEVICE} num-buffers=-1 ! "
      ;;

    libcamera)
      # default camera configuration fo libcamera source (can also be overridden by user)
      # first camera device found by cam -l is used by default if CAMERA_DEVICE is not specified
      CAMERA_INFO=$(cam -l | awk '/Available cameras:/ {getline; print}')
      CAMERA_DEVICE_DEFAULT=$(echo "$CAMERA_INFO" | sed -n "s/.*(\(.*\)).*/\1/p")
      CAMERA_DEVICE="${CAMERA_DEVICE:-${CAMERA_DEVICE_DEFAULT}}"

      if [ -z ${CAMERA_DEVICE} ]; then
        local MSG="No libcamera compatible camera device were found."
        MSG="$MSG Check device list with 'cam -l' and set CAMERA_DEVICE variable appropriately."
        error "$MSG"
      fi

      CAMERA_WIDTH="${CAMERA_WIDTH:-1920}"
      CAMERA_HEIGHT="${CAMERA_HEIGHT:-1080}"
      CAMERA_FPS="${CAMERA_FPS:-60}"

      CAMERA_SOURCE="libcamerasrc name=cam_src camera-name="${CAMERA_DEVICE}" ! queue ! "
      ;;
    esac
  fi
}
 
  # backend default configuration for ML inferences
  INFERENCE_BACKEND="${BACKEND:-NPU}"
  case "${INFERENCE_BACKEND}" in
  CPU|NPU)
    export USE_GPU_INFERENCE=0 ;;
  GPU)
    export USE_GPU_INFERENCE=1 ;;
  *)
    error "invalid inference backend ${INFERENCE_BACKEND}" ;;
  esac

  # GPU configuration for image processing
  GPU=${GPU:-GPU2D}
  # GPU2D API
  case "${IMX}" in
  IMX8MP)
    GPU2D_API="G2D" ;;
  IMX95)
    GPU2D_API="G2D" ;;
  IMX93)
    GPU2D_API="PXP" ;;
  *)
    GPU2D_API="NONE" ;;
  esac
  # GPU3D API
  case "${IMX}" in
  IMX8MP)
    GPU3D_API="OCL" ;;
  IMX95)
    GPU3D_API="OCL" ;;
  *)
    GPU3D_API="NONE" ;;
  esac


  # XXX: i.MX93: mute noisy ethos-u kernel driver
  if [ "${IMX}" = "IMX93" ]; then
    echo 4 > /proc/sys/kernel/printk
  fi
}

# Create pipeline segment for accelerated video formatting and csc
# $1 video output width
# $2 video output height
# $3 video output format
function accelerated_video_scale_str {
  local VIDEO_SCALE
  local OUTPUT_WIDTH=$1
  local OUTPUT_HEIGHT=$2
  local OUTPUT_FORMAT=$3
  local ELEMENT_NAME

  if [[ -z "${OUTPUT_FORMAT}" ]]
  then
    FORMAT=""
    ELEMENT_NAME="scale"
  else
    FORMAT=",format=${OUTPUT_FORMAT}"
    ELEMENT_NAME="scale_csc"
  fi

  case "${GPU}" in
  GPU2D)
    case "${GPU2D_API}" in
    G2D)
      # g2d-based
      VIDEO_SCALE="imxvideoconvert_g2d name=${ELEMENT_NAME}_g2d ! "
      ;;
    PXP)
      # pxp-based
      VIDEO_SCALE="imxvideoconvert_pxp name=${ELEMENT_NAME}_pxp ! "
      ;;
    *)
      # cpu-based
      VIDEO_SCALE="videoscale name=scale_cpu ! videoconvert name=csc_cpu ! "
      ;;
    esac
    ;;
  GPU3D)
    case "${GPU3D_API}" in
    OCL)
      # ocl-based
      VIDEO_SCALE="imxvideoconvert_ocl name=${ELEMENT_NAME}_ocl ! "
      ;;
    *)
      # cpu-based
      VIDEO_SCALE="videoscale name=scale_cpu ! videoconvert name=csc_cpu ! "
      ;;
    esac
    ;;
  *)
    error "invalid GPU ${GPU}" ;;
  esac


  VIDEO_SCALE+="video/x-raw,width=${OUTPUT_WIDTH},height=${OUTPUT_HEIGHT}${FORMAT} ! "

  echo "${VIDEO_SCALE}"
}


# Create pipeline segment for accelerated video scaling and conversion to RGB format
# $1 video output width
# $2 video output height
function accelerated_video_scale_rgb_str {
  local VIDEO_SCALE_RGB
  local OUTPUT_WIDTH=$1
  local OUTPUT_HEIGHT=$2
  local FORMAT_ELEMENT_NAME="rgb_convert_cpu"

case "${GPU}" in
  GPU2D)
    case "${GPU2D_API}" in
    G2D)
      # g2d-based
      if [ "${IMX}" = "IMX8MP" ]; then
      VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "RGBA")
      VIDEO_SCALE_RGB+="videoconvert name=${FORMAT_ELEMENT_NAME} ! video/x-raw,format=RGB ! "
      else
      VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "RGB")
      fi
      ;;
    PXP)
      # pxp-based
      VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "BGR")
      VIDEO_SCALE_RGB+="videoconvert name=${FORMAT_ELEMENT_NAME} ! video/x-raw,format=RGB ! "
      ;;
    *)
      # cpu-based
      VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "RGB")
      ;;
    esac
    ;;
  GPU3D)
    case "${GPU3D_API}" in
    OCL)
      # ocl-based
       VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "RGB")
      ;;
    *)
      # cpu-based
      VIDEO_SCALE_RGB=$(accelerated_video_scale_str ${MODEL_WIDTH} ${MODEL_HEIGHT} "RGB")
      ;;
  esac
    ;;
  *)
    error "invalid GPU ${GPU}" ;;
  esac

  echo "${VIDEO_SCALE_RGB}"
}


# accelerated video mixer
# $1 mixer name
# $2 mixer extra parameters
# parameters for compositor not supporting alpha channel (pxp)
# $3 alpha channel pad number
# $4 alpha channel value
# $5 max inputs latency in nanoseconds
function accelerated_video_mixer_str {
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
