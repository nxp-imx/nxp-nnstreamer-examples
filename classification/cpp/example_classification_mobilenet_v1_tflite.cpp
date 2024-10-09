/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for image classification using tensorflow-lite. 
 * The model used is mobilenet_v1_1.0_224.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * v4l2src -- tee -----------------------------------------------------------------------------------
 *             |                                                                                     |
 *             |                                                                              textoverlay -- autovideosink
 *             |                                                                                     |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"

#include <iostream>
#include <getopt.h>

// Check if command parser has an optional argument
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


const int CAMERA_INPUT_WIDTH = 640;
const int CAMERA_INPUT_HEIGHT = 480;


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path modelPath;
  std::string backend;
  std::string norm;
  DataDir dataDir;
  bool time = false;
  bool freq = false;
  std::string textColor;
  char* graphPath = getenv("HOME");
} ParserOptions;


int cmdParser(int argc, char **argv, ParserOptions& options)
{
  int c;
  int optionIndex;
  std::string perfDisplay;
  imx::Imx imx{};
  static struct option longOptions[] = {
    {"help",          no_argument,       0, 'h'},
    {"backend",       required_argument, 0, 'b'},
    {"normalization", required_argument, 0, 'n'},
    {"camera_device", required_argument, 0, 'c'},
    {"model_path",    required_argument, 0, 'p'},
    {"labels_path",   required_argument, 0, 'l'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {0,               0,                 0,   0}
  };
  
  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:l:d::t:g:",
                          longOptions,
                          &optionIndex)) != -1) {
    switch (c)
    {
      case 'h':
        std::cout << "Help Options:" << std::endl
                  << std::setw(25) << std::left << "  -h, --help"
                  << std::setw(25) << std::left << "Show help options"
                  << std::endl << std::endl
                  << "Application Options:" << std::endl

                  << std::setw(25) << std::left << "  -b, --backend"
                  << std::setw(25) << std::left
                  << "Use the selected backend (CPU,GPU,NPU)" << std::endl

                  << std::setw(25) << std::left << "  -n, --normalization"
                  << std::setw(25) << std::left
                  << "Use the selected normalization"
                  << " (none,centered,reduced,centeredReduced,castInt32,castuInt8)" << std::endl

                  << std::setw(25) << std::left << "  -c, --camera_device"
                  << std::setw(25) << std::left
                  << "Use the selected camera device (/dev/video{number})"
                  << std::endl

                  << std::setw(25) << std::left << "  -p, --model_path"
                  << std::setw(25) << std::left
                  << "Use the selected model path" << std::endl

                  << std::setw(25) << std::left << "  -l, --labels_path"
                  << std::setw(25) << std::left
                  << "Use the selected labels path" << std::endl
                  
                  << std::setw(25) << std::left << "  -d, --display_perf"
                  << std::setw(25) << std::left
                  << "Display performances, can specify time or freq" << std::endl
                  
                  << std::setw(25) << std::left << "  -t, --text_color"
                  << std::setw(25) << std::left
                  << "Color of performances displayed,"
                  << " can choose between red, green, blue, and black (white by default)" << std::endl
                  
                  << std::setw(25) << std::left << "  -g, --graph_path"
                  << std::setw(25) << std::left
                  << "Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)" << std::endl;
        return 1;

      case 'b':
        options.backend.assign(optarg);
        break;

      case 'n':
        options.norm.assign(optarg);
        break;

      case 'c':
        options.camDevice.assign(optarg);
        break;

      case 'p':
        options.modelPath.assign(optarg);
        break;

      case 'l':
        options.dataDir.labelsDir.assign(optarg);
        break;

      case 'd':
        if (OPTIONAL_ARGUMENT_IS_PRESENT)
            perfDisplay.assign(optarg);

        if (perfDisplay == "freq") {
          options.freq = true;
        } else if (perfDisplay == "time") {
          options.time = true;
        } else {
          options.time = true;
          options.freq = true;
        }
        break;

      case 't':
        options.textColor.assign(optarg);
        break;
      
      case 'g':
        if (imx.socId() != imx::IMX8MP) {
          log_error("OpenVX graph compilation only for i.MX8MPlus\n");
          return 1;
        }
        options.graphPath = optarg;
        break;

      default:
        break;
    }
  }
  return 0;
}


int main(int argc, char **argv)
{
  // Create pipeline object
  GstPipelineImx pipeline;

  // Set command line parser with default values
  ParserOptions options;
  options.backend = "NPU";
  options.norm = "none";
  if (cmdParser(argc, argv, options))
    return 0;
 
  // Add camera to pipeline
  GstCameraImx camera(options.camDevice,
                      "cam_src",
                      CAMERA_INPUT_WIDTH,
                      CAMERA_INPUT_HEIGHT,
                      false);
  camera.addCameraToPipeline(pipeline);

  // Add a tee element for parallelization of tasks
  std::string teeName = "t";
  pipeline.doInParallel(teeName);

  // Add a branch to tee element for inference and model post processing
  GstQueueOptions nnQueue = {
    .queueName     = "thread-nn",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnQueue);

  // Add model inference
  TFliteModelInfos classification(options.modelPath, options.backend, options.norm);
  classification.addInferenceToPipeline(pipeline, "classification_filter");

  // Add NNStreamer inference output decoding
  NNDecoder decoder;
  decoder.addImageLabeling(pipeline, options.dataDir.labelsDir.string());

  // Link decoder result to a text overlay
  std::string overlayName = "overlay";
  pipeline.linkToTextOverlay(overlayName);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Add text overlay and display it
  GstVideoPostProcess postProcess;
  TextOverlayOptions overlayOptions = {
    .name       = overlayName,  // Text overlay element name
    .fontName   = "Sans",       // Font name
    .fontSize   = 24,           // Font size
    .color      = "",           // Text color (white)
    .vAlignment = "baseline",   // Horizontal alignment of the text
    .hAlignment = "center",     // Vertical alignment of the text
    .text       = "",           // Text to display, set only if linkToTextOverlay is not used
  };
  postProcess.addTextOverlay(pipeline, overlayOptions);
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}