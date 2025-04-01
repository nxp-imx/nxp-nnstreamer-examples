#!/usr/bin/env bash

# SPDX-License-Identifier: MIT
# Copyright 2025 NXP

python3.10 -m venv env

source ./env/bin/activate

pip install --upgrade tflite2tensorflow

wget https://github.com/PINTO0309/tflite2tensorflow/raw/main/schema/schema.fbs

git clone -b v2.0.8 https://github.com/google/flatbuffers.git
(
cd flatbuffers && mkdir build && cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
)

pip install -r $1

tflite2tensorflow \
  --model_path midas_2_1_small_float32.tflite \
  --flatc_path flatbuffers/build/flatc \
  --schema_path schema.fbs \
  --output_pb

mv saved_model/model_float32.pb model_float32.pb

#cleanup
deactivate
rm -rf sample_npy saved_model flatbuffers env
rm schema.fbs midas_2_1_small_float32.json