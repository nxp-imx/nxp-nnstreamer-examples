# Copyright 2023-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required( VERSION 3.0 )
project( nxp-nnstreamer-examples )

# Specify the C++ standard
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

# Add GStreamer library
find_package( PkgConfig )
pkg_check_modules( GSTREAMER REQUIRED gstreamer-1.0 )
pkg_check_modules( GSTREAMER REQUIRED gstreamer-app-1.0 )
pkg_check_modules( GSTREAMER REQUIRED gstreamer-pbutils-1.0 )
include_directories( ${GSTREAMER_INCLUDE_DIRS} )
link_directories( ${GSTREAMER_LIBRARY_DIRS} )

# Add Cairo library
pkg_check_modules( CAIRO REQUIRED cairo )
include_directories( ${CAIRO_INCLUDE_DIRS} )
link_directories( ${CAIRO_LIBRARY_DIRS} )

# Add Custom library
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/common/cpp/include )
file( GLOB all_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/common/cpp/src/*.cpp" )

# Check OpenMP library
find_package(OpenMP REQUIRED)

# Example of object classification (mobilenet_v1)
add_executable(
  example_classification_mobilenet_v1_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/classification/cpp/example_classification_mobilenet_v1_tflite.cpp
)
target_link_libraries(
  example_classification_mobilenet_v1_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_classification_mobilenet_v1_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./classification )

# Example of object detection (mobilenet_ssd)
add_executable(
  example_detection_mobilenet_ssd_v2_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/object-detection/cpp/example_detection_mobilenet_ssd_v2_tflite.cpp
)
target_link_libraries(
  example_detection_mobilenet_ssd_v2_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_detection_mobilenet_ssd_v2_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./object-detection )

# Example of object classification (mobilenet_v1) and object detection (mobilenet_ssd) in parallel
add_executable(
  example_classification_and_detection_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/mixed-demos/cpp/example_classification_and_detection_tflite.cpp
)
target_link_libraries(
  example_classification_and_detection_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_classification_and_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed-demos )

# Example of object segmentation (deeplab_v3)
add_executable(
  example_segmentation_deeplab_v3_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/semantic-segmentation/cpp/example_segmentation_deeplab_v3_tflite.cpp
)
target_link_libraries(
  example_segmentation_deeplab_v3_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_segmentation_deeplab_v3_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./semantic-segmentation )

# Example of pose detection (PoseNet Lightning)
add_executable(
  example_pose_movenet_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/pose-estimation/cpp/example_pose_movenet_tflite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/pose-estimation/cpp/custom_pose_decoder.cpp
)
target_include_directories( example_pose_movenet_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tasks/pose-estimation/cpp/ )
target_link_libraries(
  example_pose_movenet_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_pose_movenet_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./pose-estimation )

# Example of face detection (UltraFace slim)
add_executable(
  example_face_detection_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/face-detection/cpp/example_face_detection_tflite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/face-detection/cpp/custom_face_decoder.cpp
)
target_include_directories( example_face_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/face-detection/cpp/ )
target_link_libraries(
  example_face_detection_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_face_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./face-processing )

# Example of face detection (UltraFace slim) and pose detection (PoseNet Lightning) in parallel
add_executable(
  example_face_and_pose_detection_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/mixed-demos/cpp/example_face_and_pose_detection_tflite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/mixed-demos/cpp/custom_face_and_pose_decoder.cpp)
target_include_directories( example_face_and_pose_detection_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tasks/mixed-demos/cpp/ )
target_link_libraries(
  example_face_and_pose_detection_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_face_and_pose_detection_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed-demos )

# Example of double object classification (mobilenet_v1)
add_executable(
  example_double_classification_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/mixed-demos/cpp/example_double_classification_tflite.cpp
)
target_link_libraries(
  example_double_classification_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_double_classification_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./mixed-demos )

# Example of emotion classification (UltraFace slim, and Deepface-emotion)
add_executable(
  example_emotion_classification_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/emotion-classification/cpp/example_emotion_classification_tflite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/emotion-classification/cpp/custom_emotion_decoder.cpp
)
target_include_directories( example_emotion_classification_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tasks/face-processing/emotion-classification/cpp/ )
target_link_libraries(
  example_emotion_classification_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
)
set_target_properties( example_emotion_classification_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./face-processing )

# Example of depth estimation (midas_v2)
add_executable(
  example_depth_midas_v2_tflite
  ${all_SRCS}
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/monocular-depth-estimation/cpp/example_depth_midas_v2_tflite.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tasks/monocular-depth-estimation/cpp/custom_depth_decoder.cpp
)
target_include_directories( example_depth_midas_v2_tflite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tasks/monocular-depth-estimation/cpp/ )
target_link_libraries(
  example_depth_midas_v2_tflite
  ${GSTREAMER_LIBRARIES}
  ${CAIRO_LIBRARIES}
  tensorflow-lite
  "${OpenMP_CXX_FLAGS}"
)
set_target_properties( example_depth_midas_v2_tflite PROPERTIES RUNTIME_OUTPUT_DIRECTORY ./monocular-depth-estimation )
target_compile_options( example_depth_midas_v2_tflite PRIVATE "${OpenMP_CXX_FLAGS}" )
