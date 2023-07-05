#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2023 NXP

set -x

REALPATH="$(readlink -e "$0")"
BASEDIR="$(dirname "${REALPATH}")/.."
MODELS_DIR="${BASEDIR}/downloads/models/detection"

source "${BASEDIR}/common/common_utils.sh"
source "${BASEDIR}/detection/detection_utils.sh"

setup_env

# model and framework dependant variables 
declare -A MODEL_BACKEND
declare -A MODEL_BACKEND_NPU
MODEL_BACKEND_NPU[IMX8MP]="${MODELS_DIR}/yolov4-tiny_416_quant.tflite"
MODEL_BACKEND_NPU[IMX93]="${MODELS_DIR}/yolov4-tiny_416_quant_vela.tflite"

MODEL_BACKEND[CPU]="${MODELS_DIR}/yolov4-tiny_416.tflite"
MODEL_BACKEND[NPU]=${MODEL_BACKEND_NPU[${IMX}]}
MODEL=${MODEL_BACKEND[${BACKEND}]}

declare -A MODEL_LATENCY_NPU_NS
MODEL_LATENCY_NPU_NS[IMX8MP]="50000000"
MODEL_LATENCY_NPU_NS[IMX93]="100000000"

declare -A MODEL_LATENCY_NS
MODEL_LATENCY_NS[CPU]="300000000"
MODEL_LATENCY_NS[NPU]=${MODEL_LATENCY_NPU_NS[${IMX}]}
MODEL_LATENCY=${MODEL_LATENCY_NS[${BACKEND}]}

MODEL_WIDTH=416
MODEL_HEIGHT=416
MODEL_LABELS="${MODELS_DIR}/coco-labels-2014_2017.txt"


FRAMEWORK="tensorflow-lite"

# tensor filter configuration
FILTER_COMMON="tensor_filter framework=${FRAMEWORK} model=${MODEL}"

declare -A FILTER_BACKEND_NPU
FILTER_BACKEND_NPU[IMX8MP]=" custom=Delegate:External,ExtDelegateLib:libvx_delegate.so ! "
FILTER_BACKEND_NPU[IMX93]=" custom=Delegate:External,ExtDelegateLib:libethosu_delegate.so ! "

declare -A FILTER_BACKEND
FILTER_BACKEND[CPU]="${FILTER_COMMON}"
FILTER_BACKEND[CPU]+=" custom=Delegate:XNNPACK,NumThreads:$(nproc --all) !"
FILTER_BACKEND[NPU]="${FILTER_COMMON}"
FILTER_BACKEND[NPU]+=${FILTER_BACKEND_NPU[${IMX}]}
TENSOR_FILTER=${FILTER_BACKEND[${BACKEND}]}

# python filter configuration
FRAMEWORK="python3"
POSTPROCESS="${BASEDIR}/detection/postprocess_yolov4_tiny.py"
MODEL_SIZE="Height:${MODEL_HEIGHT},Width:${MODEL_WIDTH}"
THRESHOLD="Threshold:0.4" # optional argument between 0 and 1
if [ -z "${THRESHOLD}" ];
then
    TENSOR_FILTER+=" tensor_filter framework=${FRAMEWORK} model=${POSTPROCESS} custom=${MODEL_SIZE} ! ";
else
    TENSOR_FILTER+=" tensor_filter framework=${FRAMEWORK} model=${POSTPROCESS} custom=${MODEL_SIZE},${THRESHOLD} ! ";
fi

# tensor preprocessing configuration: normalize video for float input models
declare -A PREPROCESS_BACKEND
PREPROCESS_BACKEND[CPU]="tensor_transform mode=arithmetic option=typecast:float32,div:255 !"
PREPROCESS_BACKEND[NPU]="tensor_transform mode=arithmetic option=typecast:int8,add:-128 ! "
TENSOR_PREPROCESS=${PREPROCESS_BACKEND[${BACKEND}]}


# tensor decoder configuration: yolov4-tiny without post processing
TENSOR_DECODER="tensor_decoder mode=bounding_boxes"
TENSOR_DECODER+=" option1=yolov5"
TENSOR_DECODER+=" option2=${MODEL_LABELS}"
# option3 is already set to tensorflow framework by default
TENSOR_DECODER+=" option4=${CAMERA_WIDTH}:${CAMERA_HEIGHT}"
TENSOR_DECODER+=" option5=${MODEL_WIDTH}:${MODEL_HEIGHT} !"

gst_exec_detection

