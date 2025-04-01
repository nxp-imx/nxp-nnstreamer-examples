#!/bin/bash
#
# Copyright 2024-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

export REALPATH="$(readlink -f -- "$0")"
BASEDIR="$(dirname "${REALPATH}")"
MODELS_DIR="${BASEDIR}/downloads/models"
MEDIA_DIR="${BASEDIR}/downloads/media"

# Define media
export POWER_JUMP_VIDEO="${MEDIA_DIR}/movies/Conditioning_Drill_1-_Power_Jump.webm.480p.vp9.webm"
export PASCAL_IMAGES="${MEDIA_DIR}/pascal_voc_2012_images/image%04d.jpg"
export SAVE_VIDEO_PATH="/root/video.mkv"

# Define cameras path for two cameras classification demo
export CAM1_PATH="/dev/video0"
export CAM2_PATH="/dev/video2"

# Define classification data path
CLASSIFICATION_DIR="${MODELS_DIR}/classification"
export MOBILENETV1_LABELS="${CLASSIFICATION_DIR}/labels_mobilenet_quant_v1_224.txt"
export MOBILENETV1="${CLASSIFICATION_DIR}/mobilenet_v1_1.0_224.tflite"
export MOBILENETV1_QUANT="${CLASSIFICATION_DIR}/mobilenet_v1_1.0_224_quant_uint8_float32.tflite"
export MOBILENETV1_QUANT_VELA="${CLASSIFICATION_DIR}/mobilenet_v1_1.0_224_quant_uint8_float32_vela.tflite"
export MOBILENETV1_QUANT_NEUTRON="${CLASSIFICATION_DIR}/mobilenet_v1_1.0_224_quant_uint8_float32_neutron.tflite"

# Define depth data path
DEPTH_DIR="${MODELS_DIR}/monocular-depth-estimation"
export MIDASV2="${DEPTH_DIR}/midas_2_1_small_int8_quant.tflite"
export MIDASV2_VELA="${DEPTH_DIR}/midas_2_1_small_int8_quant_vela.tflite"

# Define detection data path
DETECTION_DIR="${MODELS_DIR}/object-detection"
export COCO_LABELS="${DETECTION_DIR}/coco_labels_list.txt"
export MOBILENETV2_BOXES="${DETECTION_DIR}/box_priors.txt"
export MOBILENETV2="${DETECTION_DIR}/ssdlite_mobilenet_v2_coco_no_postprocess.tflite"
export MOBILENETV2_QUANT="${DETECTION_DIR}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess.tflite"
export MOBILENETV2_QUANT_VELA="${DETECTION_DIR}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess_vela.tflite"
export MOBILENETV2_QUANT_NEUTRON="${DETECTION_DIR}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess_neutron.tflite"

# Define face data path
FACE_DIR="${MODELS_DIR}/face-processing"
export ULTRAFACE_QUANT="${FACE_DIR}/ultraface_slim_uint8_float32.tflite"
export ULTRAFACE_QUANT_VELA="${FACE_DIR}/ultraface_slim_uint8_float32_vela.tflite"
export ULTRAFACE_QUANT_NEUTRON="${FACE_DIR}/ultraface_slim_uint8_float32_neutron.tflite"
export EMOTION_QUANT="${FACE_DIR}/emotion_uint8_float32.tflite"
export EMOTION_QUANT_VELA="${FACE_DIR}/emotion_uint8_float32_vela.tflite"

# Define pose data path
POSE_DIR="${MODELS_DIR}/pose-estimation"
export MOVENET="${POSE_DIR}/movenet_single_pose_lightning.tflite"
export MOVENET_QUANT="${POSE_DIR}/movenet_quant.tflite"
export MOVENET_QUANT_VELA="${POSE_DIR}/movenet_quant_vela.tflite"

# Define segmentation data path
SEGMENTATION_DIR="${MODELS_DIR}/semantic-segmentation"
export DEEPLABV3="${SEGMENTATION_DIR}/deeplabv3_mnv2_dm05_pascal.tflite"
export DEEPLABV3_QUANT="${SEGMENTATION_DIR}/deeplabv3_mnv2_dm05_pascal_quant_uint8_float32.tflite"
export DEEPLABV3_QUANT_VELA="${SEGMENTATION_DIR}/deeplabv3_mnv2_dm05_pascal_quant_uint8_float32_vela.tflite"