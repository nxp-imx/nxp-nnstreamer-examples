#!/bin/bash
# SPDX-License-Identifier:BSD-3-Clause
# Copyright 2022 NXP

set -x

REALPATH="$(readlink -e "$0")"
BASEDIR="$(dirname "${REALPATH}")/.."
BASEDIR="$(realpath ${BASEDIR})"

function usage {
  echo "Upload models and artifact to the target for later execution."
  echo "syntax:"
  echo "upload <board username>@<board ip address> [remote path]"
  echo "example:"
  echo "upload root@192.168.1.52"
}

REMOTE="$1"
if [ -z "${REMOTE}" ]; then
    usage
    exit 1
fi

REMOTE_PATH=$2
if [ -z "${REMOTE_PATH}" ]; then
    REMOTE_USER="${REMOTE%@*}"
    REMOTE_PATH="/home/${REMOTE_USER}"
fi
REMOTE_DIR="${REMOTE_PATH}/"$(basename ${BASEDIR})

# exclude unnecessary directories
ssh "${REMOTE}" "rm -rf ${REMOTE_DIR} && mkdir -p ${REMOTE_DIR}"
find "${BASEDIR}" -maxdepth 1 -mindepth 1 -type d \
    ! -path '*/.git' \
    ! -path '*/samples' \
    ! -path '*/tmp' \
    ! -path '*/tools' \
     -exec scp -r {} ${REMOTE}:${REMOTE_DIR} \;
