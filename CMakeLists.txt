# Copyright 2023 NXP
# SPDX-License-Identifier: BSD-3-Clause 


cmake_minimum_required(VERSION 3.0)

project(nxp-nnstreamer-examples-C++)

find_package(PkgConfig) 

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

include_directories(
        ${GSTREAMER_INCLUDE_DIRS}
)

link_directories(
        ${GSTREAMER_LIBRARY_DIRS}
)

add_executable(example_classification_mobilenet_v1_tflite classification/example_classification_mobilenet_v1_tflite.cc)
add_definitions(-DCLASSIFICATION_MODELS_DEFAULT_PATH="/tmp/models")
target_include_directories(example_classification_mobilenet_v1_tflite PRIVATE ./common/cpp)
target_link_libraries(example_classification_mobilenet_v1_tflite ${GSTREAMER_LIBRARIES})
set_target_properties(example_classification_mobilenet_v1_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./classification)
