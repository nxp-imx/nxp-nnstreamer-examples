/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for depth estimation using tensorflow-lite.
 * The model used is [...] which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 * 
 * Pipeline:
 * multifilesrc -- jpegdec -- imxvideoconvert -- tee -----------------------------------------------------------------
 *                                                |                                                                  |
 *                                                |                                                             videomixer -- autovideosink
 *                                                |                                                                  |
 *                                                --- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"
#include "custom_depth_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <algorithm>

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
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:d::t:g:",
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
  options.norm = "reduced";
  if (cmdParser(argc, argv, options))
    return 0;
  
  imx::Imx imx{};
  if ((imx.socId() == imx::IMX95) && (options.backend == "NPU")) {
    log_error("Example can't run on NPU in i.MX95\n");
    return 0;
  }

  // Add slideshow to pipeline
  GstCameraImx camera(options.camDevice,
                      "cam_src",
                      CAMERA_INPUT_WIDTH,
                      CAMERA_INPUT_HEIGHT,
                      false);
  camera.addCameraToPipeline(pipeline);

  // Add model inference
  TFliteModelInfos depthEstimation(options.modelPath,
                                   options.backend,
                                   options.norm);
  depthEstimation.addInferenceToPipeline(pipeline, "depth_filter");

  // Add tensor sink to get inference output and process it
  // after pipeline parsing
  std::string tensorSinkName = "tsink_fd";
  pipeline.addTensorSink(tensorSinkName);

  // Create pipeline object
  GstPipelineImx displayPipeline;

  // Add appsrc element to retrieve the processed image
  GstAppSrcImx appsrc("appsrc_video",
                      true,
                      false,
                      1,
                      GstQueueLeaky::downstream,
                      3,
                      256,
                      256,
                      "GRAY8",
                      1);
  appsrc.addAppSrcToPipeline(displayPipeline);

  // Add video transform because cairooverlay from enablePerfDisplay doesn't accept GRAY8
  GstVideoImx gstvideoimx;
  gstvideoimx.videoTransform(displayPipeline, "", -1, -1, false, false, true);

  // Display processed video
  pipeline.enablePerfDisplay(options.freq, options.time, 10, options.textColor);
  GstVideoPostProcess postProcess;
  postProcess.display(displayPipeline, false);

  // Parse pipeline to GStreamer pipeline
  displayPipeline.parse(argc, argv, options.graphPath);
  pipeline.parse(argc, argv, options.graphPath);

  // Connect callback functions
  DecoderData boxesData;
  boxesData.appSrc = displayPipeline.getElement("appsrc_video");
  pipeline.connectToElementSignal(tensorSinkName, newDataCallback, "new-data", &boxesData);

  // Run GStreamer pipeline
  displayPipeline.run();
  pipeline.run();

  return 0;
}