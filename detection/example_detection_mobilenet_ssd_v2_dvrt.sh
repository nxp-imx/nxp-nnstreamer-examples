#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022 NXP

set -x

REALPATH="$(readlink -e "$0")"
BASEDIR="$(dirname "${REALPATH}")/.."
MODELS_DIR="${BASEDIR}/downloads/models/detection"

source "${BASEDIR}/common/common_utils.sh"
source "${BASEDIR}/detection/detection_utils.sh"

setup_env

# model and framework dependant variables 
declare -A MODEL_BACKEND
MODEL_BACKEND[CPU]="${MODELS_DIR}/ssdlite_mobilenet_v2_coco_no_postprocess.rtm"
MODEL_BACKEND[GPU]="${MODELS_DIR}/ssdlite_mobilenet_v2_coco_no_postprocess.rtm"
MODEL_BACKEND[NPU]="${MODELS_DIR}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess.rtm"
MODEL=${MODEL_BACKEND[${BACKEND}]}

declare -A MODEL_LATENCY_NS
MODEL_LATENCY_NS[CPU]="500000000"
MODEL_LATENCY_NS[GPU]="500000000"
MODEL_LATENCY_NS[NPU]="0"
MODEL_LATENCY=${MODEL_LATENCY_NS[${BACKEND}]}

MODEL_WIDTH=300
MODEL_HEIGHT=300
MODEL_BOXES="${MODELS_DIR}/box_priors.txt"
MODEL_LABELS="${MODELS_DIR}/coco_labels_list.txt"

FRAMEWORK="deepview-rt"

# tensor filter configuration
FILTER_COMMON="tensor_filter framework=${FRAMEWORK} model=${MODEL}"
declare -A FILTER_BACKEND
FILTER_BACKEND[CPU]="${FILTER_COMMON}"
FILTER_BACKEND[CPU]+=" ! "
FILTER_BACKEND[GPU]="${FILTER_COMMON}"
FILTER_BACKEND[GPU]+=" custom=Engine:/usr/lib/deepview-rt-openvx.so ! "
FILTER_BACKEND[NPU]="${FILTER_COMMON}"
FILTER_BACKEND[NPU]+=" custom=Engine:/usr/lib/deepview-rt-openvx.so ! "
TENSOR_FILTER=${FILTER_BACKEND[${BACKEND}]}

# tensor preprocessing configuration: normalize video for float input models
declare -A PREPROCESS_BACKEND
PREPROCESS_BACKEND[CPU]="tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! "
PREPROCESS_BACKEND[GPU]="tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! "
PREPROCESS_BACKEND[NPU]=""
TENSOR_PREPROCESS=${PREPROCESS_BACKEND[${BACKEND}]}

# tensor decoder configuration: mobilenet ssd without post processing
TENSOR_DECODER="tensor_decoder mode=bounding_boxes"
TENSOR_DECODER+=" option1=mobilenet-ssd"
TENSOR_DECODER+=" option2=${MODEL_LABELS}"
TENSOR_DECODER+=" option3=${MODEL_BOXES}"
TENSOR_DECODER+=" option4=${CAMERA_WIDTH}:${CAMERA_HEIGHT}"
TENSOR_DECODER+=" option5=${MODEL_WIDTH}:${MODEL_HEIGHT} ! "

if [ "${IMX}" = "IMX93" ] && [ "${BACKEND}" != "CPU" ]; then
    error "${BACKEND} not supported for DeepViewRT - set BACKEND=CPU"
fi

gst_exec_detection

