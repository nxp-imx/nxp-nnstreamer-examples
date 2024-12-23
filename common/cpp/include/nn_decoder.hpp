/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPP_NN_DECODER_H_
#define CPP_NN_DECODER_H_

#include <filesystem>
#include <map>

#include "gst_pipeline_imx.hpp"
#include "tensor_custom_data_generator.hpp"


/**
 * @brief Enum of available models for bounding_boxes mode.
 */
enum class ModeBoundingBoxes {
  yolov5,
  mobilenetssd,
  mpPalmDetection,
};


/**
 * @brief Dimension structure.
 */
typedef struct {
  int width;
  int height;
} Dimension;


/**
 * @brief Structure for bounding_boxes mode options.
 */
typedef struct {
  ModeBoundingBoxes modelName;
  std::filesystem::path labelsPath;
  std::string option3; // Option1-dependent values
  Dimension outDim;
  Dimension inDim;
  bool trackResult;
  bool logResult;
} BoundingBoxesOptions;


/**
 * @brief Yolov5 custom options for bounding_boxes mode.
 */
typedef struct {
  int scale;
  float confidence = -1;              //optional
  float IOU = -1;                     //optional
} YoloCustomOptions;


/**
 * @brief Mobilenetssd custom options for bounding_boxes mode.
 */
typedef struct {
  std::filesystem::path boxesPath;
  float threshold = -1;               //optional
  float yScale = -1;                  //optional
  float xScale = -1;                  //optional
  float hScale = -1;                  //optional
  float wScale = -1;                  //optional
  float IOU = -1;                     //optional
} SSDMobileNetCustomOptions;


/**
 * @brief Palm detection custom options for bounding_boxes mode.
 */
typedef struct {
  float score;
  int anchorLayers = -1;              //optional
  float minScale = -1;                //optional
  float maxScale = -1;                //optional
  float xOffset = -1;                 //optional
  float yOffset = -1;                 //optional
  std::string stride = "";            //optional
} PalmDetectionCustomOptions;


/**
 * @brief Set custom options for bounding_boxes mode between available models.
 */
template<typename T>
std::string setCustomOptions(T options) {
  std::string cmd;
  if constexpr(std::is_same_v<T, YoloCustomOptions>) {
    cmd += std::to_string(options.scale);
    if (options.confidence != -1)
      cmd += ":" + std::to_string(options.confidence);
    if (options.IOU != -1)
      cmd += ":" + std::to_string(options.IOU);
  }
  if constexpr(std::is_same_v<T, SSDMobileNetCustomOptions>) {
    cmd += options.boxesPath.string();
    if (options.threshold != -1)
      cmd += ":" + std::to_string(options.threshold);
    if (options.yScale != -1)
      cmd += ":" + std::to_string(options.yScale);
    if (options.xScale != -1)
      cmd += ":" + std::to_string(options.xScale);
    if (options.hScale != -1)
      cmd += ":" + std::to_string(options.hScale);
    if (options.wScale != -1)
      cmd += ":" + std::to_string(options.wScale);
    if (options.IOU != -1)
      cmd += ":" + std::to_string(options.IOU);
  }
  if constexpr(std::is_same_v<T, PalmDetectionCustomOptions>) {
    cmd += std::to_string(options.score);
    if (options.anchorLayers != -1)
      cmd += ":" + std::to_string(options.anchorLayers);
    if (options.minScale != -1)
      cmd += ":" + std::to_string(options.minScale);
    if (options.maxScale != -1)
      cmd += ":" + std::to_string(options.maxScale);
    if (options.xOffset != -1)
      cmd += ":" + std::to_string(options.xOffset);
    if (options.yOffset != -1)
      cmd += ":" + std::to_string(options.yOffset);
    if (options.stride != "")
      cmd += ":" + options.stride;
  }
  return cmd;
}


/**
 * @brief Enum of available models for image_segment mode.
 */
enum class ModeImageSegment {
  tfliteDeeplab,
  snpeDeeplab,
  snpeDepth,
};


/**
 * @brief Structure for image_segment mode options.
 */
typedef struct {
  ModeImageSegment modelName;
  int numClass = -1;                  //optional
} ImageSegmentOptions;


/**
 * @brief Create pipeline segments for NNStreamer decoder.
 */
class NNDecoder {
  public:
    void addImageSegment(GstPipelineImx &pipeline,
                         const ImageSegmentOptions &options);

    void addImageLabeling(GstPipelineImx &pipeline,
                          const std::filesystem::path &labelsPath);

    void addBoundingBoxes(GstPipelineImx &pipeline,
                          const BoundingBoxesOptions &options);
};
#endif