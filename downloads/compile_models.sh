#!/bin/bash
#
# Copyright 2023-2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

set -x

REALPATH="$(readlink -e "$0")"
BASEDIR="${BASEDIR:-$(dirname "${REALPATH}")/..}"
MODELS_DIR="${BASEDIR}/downloads/models"
REQUIRED_CAMERA=0

# List of target devices to exclude from compilation
EXCLUDED_DEVICES=("imx95" "imx952")

function compile_models(){
# Compile a model if required by NPU
# Argument must be destination directory ($1)

  # Convert quantized TFLite models to vela for i.MX 93 Ethos-U NPU
  if [[ "${IMX}" == "IMX93" ]]
  then
    # Convert only quantized models to vela
    DESTINATION_DIR="${1}"

    # Build grep exclusion pattern from EXCLUDED_DEVICES array
    EXCLUDE_PATTERN=""
    for DEVICE in "${EXCLUDED_DEVICES[@]}"
    do
      EXCLUDE_PATTERN="${EXCLUDE_PATTERN} -e ${DEVICE}"
    done

    for FILE in $(find "${DESTINATION_DIR}" -name "*uint8*.tflite" -o -name "*quant*.tflite" | grep -v "vela" | grep -v ${EXCLUDE_PATTERN})
    do
      vela "${FILE}" --output-dir "${DESTINATION_DIR}/output"
    done
    mv "${DESTINATION_DIR}"/output/*.tflite "${DESTINATION_DIR}"
    rm -r "${DESTINATION_DIR}/output"
  fi
}

source "${BASEDIR}/common/common_utils.sh"
setup_env

# Compile all the models once
TASKS=(classification object-detection semantic-segmentation pose-estimation face-processing monocular-depth-estimation)
for TASK in "${TASKS[@]}"
do
  DEST_DIR="${MODELS_DIR}/${TASK}"
  compile_models "${DEST_DIR}"
done
