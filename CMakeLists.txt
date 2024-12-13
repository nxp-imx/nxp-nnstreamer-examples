# Copyright 2023-2024 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.0)
project(nxp-nnstreamer-examples)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add GStreamer library
find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-app-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})
link_directories(${GSTREAMER_LIBRARY_DIRS})

# Add Cairo library
pkg_check_modules(CAIRO REQUIRED cairo)
include_directories(${CAIRO_INCLUDE_DIRS})
link_directories(${CAIRO_LIBRARY_DIRS})

#Add Custom library
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/cpp/include)
file(GLOB all_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/common/cpp/src/*.cpp")

# Example of object classification (mobilenet_v1)
add_executable(example_classification_mobilenet_v1_tflite ${all_SRCS} classification/cpp/example_classification_mobilenet_v1_tflite.cpp)
target_link_libraries(example_classification_mobilenet_v1_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_classification_mobilenet_v1_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./classification)

# Example of object detection (mobilenet_ssd)
add_executable(example_detection_mobilenet_ssd_v2_tflite ${all_SRCS} detection/cpp/example_detection_mobilenet_ssd_v2_tflite.cpp)
target_link_libraries(example_detection_mobilenet_ssd_v2_tflite  ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_detection_mobilenet_ssd_v2_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./detection)

# Example of object classification (mobilenet_v1) and object detection (mobilenet_ssd) in parallel
add_executable(example_classification_and_detection_tflite ${all_SRCS} mixed/cpp/example_classification_and_detection_tflite.cpp)
target_link_libraries(example_classification_and_detection_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_classification_and_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed)

# Example of object segmentation (deeplab_v3)
add_executable(example_segmentation_deeplab_v3_tflite ${all_SRCS} segmentation/cpp/example_segmentation_deeplab_v3_tflite.cpp)
target_link_libraries(example_segmentation_deeplab_v3_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_segmentation_deeplab_v3_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./segmentation)

# Example of pose detection (PoseNet Lightning)
add_executable(example_pose_movenet_tflite ${all_SRCS} pose/cpp/example_pose_movenet_tflite.cpp pose/cpp/custom_pose_decoder.cpp)
target_include_directories(example_pose_movenet_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pose/cpp/)
target_link_libraries(example_pose_movenet_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_pose_movenet_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./pose)

# Example of face detection (UltraFace slim)
add_executable(example_face_detection_tflite ${all_SRCS} face/cpp/example_face_detection_tflite.cpp face/cpp/custom_face_decoder.cpp)
target_include_directories(example_face_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/face/cpp/)
target_link_libraries(example_face_detection_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_face_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./face)

# Example of face detection (UltraFace slim) and pose detection (PoseNet Lightning) in parallel
add_executable(example_face_and_pose_detection_tflite ${all_SRCS} mixed/cpp/example_face_and_pose_detection_tflite.cpp mixed/cpp/custom_face_and_pose_decoder.cpp)
target_include_directories(example_face_and_pose_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mixed/cpp/)
target_link_libraries(example_face_and_pose_detection_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_face_and_pose_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed)

# Example of object classification (mobilenet_v1) on 2 cameras
add_executable(example_classification_two_cameras_tflite ${all_SRCS} classification/cpp/example_classification_two_cameras_tflite.cpp)
target_link_libraries(example_classification_two_cameras_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_classification_two_cameras_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./classification)

# Example of emotion detection (UltraFace slim, and Deepface-emotion)
add_executable(example_emotion_detection_tflite ${all_SRCS} face/cpp/example_emotion_detection_tflite.cpp face/cpp/custom_emotion_decoder.cpp)
target_include_directories(example_emotion_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/face/cpp/)
target_link_libraries(example_emotion_detection_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_emotion_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./face)

# Example of multiple pipelines (a pipeline with emotion detection, and a pipeline with object detection)
add_executable(example_emotion_and_detection_tflite ${all_SRCS} mixed/cpp/example_emotion_and_detection_tflite.cpp face/cpp/custom_emotion_decoder.cpp)
target_include_directories(example_emotion_and_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/face/cpp/)
target_link_libraries(example_emotion_and_detection_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_emotion_and_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed)

# Example of depth estimation (midas_v2)
add_executable(example_depth_midas_v2_tflite ${all_SRCS} depth/cpp/example_depth_midas_v2_tflite.cpp depth/cpp/custom_depth_decoder.cpp)
target_include_directories(example_depth_midas_v2_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/depth/cpp/)
target_link_libraries(example_depth_midas_v2_tflite ${GSTREAMER_LIBRARIES} ${CAIRO_LIBRARIES} tensorflow-lite)
set_target_properties(example_depth_midas_v2_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./depth)
