/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "nn_decoder.hpp"

/**
 * @brief Map of models available for bounding boxes mode.
 */
std::map<ModeBoundingBoxes, std::string> mapModeBoundingBoxes = {
  {ModeBoundingBoxes::yolov5, "yolov5"},
  {ModeBoundingBoxes::mobilenetssd, "mobilenet-ssd"},
  {ModeBoundingBoxes::mpPalmDetection, "mp-palm-detection"},
};


/**
 * @brief Map of models available for image segmentation mode.
 */
std::map<ModeImageSegment, std::string> mapModeImageSegment = {
  {ModeImageSegment::tfliteDeeplab, "tflite-deeplab"},
  {ModeImageSegment::snpeDeeplab, "snpe-deeplab"},
  {ModeImageSegment::snpeDepth, "snpe-depth"},
};


/**
 * @brief Add NNStreamer decoder for image segmentation.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param options: ImageSegmentOptions structure, setup image segmetation options.
 */
void NNDecoder::addImageSegment(GstPipelineImx &pipeline,
                                const ImageSegmentOptions &options)
{
  std::string cmd;
  cmd = "tensor_decoder mode=image_segment option1=";
  cmd += mapModeImageSegment[options.modelName];
  if (options.numClass != -1) {
    cmd += " option2=" + std::to_string(options.numClass) + "! ";
  } else {
    cmd += " ! videoconvert ! ";
  }
  pipeline.addToPipeline(cmd);
}


/**
 * @brief Add NNStreamer decoder for image labeling / classification.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param labelsPath: model labels path.
 */
void NNDecoder::addImageLabeling(GstPipelineImx &pipeline,
                                 const std::filesystem::path &labelsPath)
{
  std::string cmd;
  cmd = "tensor_decoder mode=image_labeling option1=" + labelsPath.string() + " ! ";
  pipeline.addToPipeline(cmd);
}


/**
 * @brief Add NNStreamer decoder for bounding boxes.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param options: BoundingBoxesOptions structure, setup bouding boxes options.
 */
void NNDecoder::addBoundingBoxes(GstPipelineImx &pipeline,
                                 const BoundingBoxesOptions &options)
{
  std::string cmd;
  cmd = "tensor_decoder mode=bounding_boxes option1=";
  cmd += mapModeBoundingBoxes[options.modelName];
  cmd += " option2=" + options.labelsPath.string();
  cmd += " option3=" + options.option3;
  cmd += " option4=" + std::to_string(options.outDim.width) + ":";
  cmd += std::to_string(options.outDim.height);
  cmd += " option5=" + std::to_string(options.inDim.width) + ":";
  cmd += std::to_string(options.inDim.height);
  if (options.trackResult == true)
    cmd += " option6=1"; 
  if (options.logResult == true)
    cmd += " option7=1";

  cmd += " ! videoconvert ! ";
  pipeline.addToPipeline(cmd);
}