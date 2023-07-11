#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022 NXP

set -x

REALPATH="$(readlink -e "$0")"
BASEDIR="$(dirname "${REALPATH}")/.."
MODELS_DIR="${BASEDIR}/downloads/models/segmentation"
MEDIA_DIR="${BASEDIR}/downloads/media"
IMAGES_DIR="${MEDIA_DIR}/pascal_voc_2012_images"
REQUIRED_CAMERA=0

source "${BASEDIR}/common/common_utils.sh"
source "${BASEDIR}/segmentation/segmentation_utils.sh"

setup_env

# model and framework dependant variables 
declare -A MODEL_BACKEND
MODEL_BACKEND[CPU]="${MODELS_DIR}/deeplabv3_mnv2_dm05_pascal.tflite"
MODEL_BACKEND[GPU]="${MODELS_DIR}/deeplabv3_mnv2_dm05_pascal.tflite"
MODEL_BACKEND[NPU]="${MODELS_DIR}/deeplabv3_mnv2_dm05_pascal_quant_uint8_float32.tflite"
MODEL=${MODEL_BACKEND[${BACKEND}]}

MODEL_WIDTH=513
MODEL_HEIGHT=513

FRAMEWORK="tensorflow-lite"

# tensor filter configuration
FILTER_COMMON="tensor_filter framework=${FRAMEWORK} model=${MODEL}"
declare -A FILTER_BACKEND
FILTER_BACKEND[CPU]="${FILTER_COMMON}"
FILTER_BACKEND[CPU]+=" custom=Delegate:XNNPACK,NumThreads:$(nproc --all) !"
FILTER_BACKEND[GPU]="${FILTER_COMMON}"
FILTER_BACKEND[GPU]+=" custom=Delegate:External,ExtDelegateLib:libvx_delegate.so ! "
FILTER_BACKEND[NPU]="${FILTER_COMMON}"
FILTER_BACKEND[NPU]+=" custom=Delegate:External,ExtDelegateLib:libvx_delegate.so ! "
TENSOR_FILTER=${FILTER_BACKEND[${BACKEND}]}

# tensor preprocessing configuration: normalize video for float input models
declare -A PREPROCESS_BACKEND
PREPROCESS_BACKEND[CPU]="tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! "
PREPROCESS_BACKEND[GPU]="tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! "
PREPROCESS_BACKEND[NPU]=""
TENSOR_PREPROCESS=${PREPROCESS_BACKEND[${BACKEND}]}

# tensor decoder configuration: tflite-deeplab (no maxargs on classes)
TENSOR_DECODER="tensor_decoder mode=image_segment option1=tflite-deeplab ! "

gst_exec_segmentation

