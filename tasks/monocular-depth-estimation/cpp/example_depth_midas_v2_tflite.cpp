/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for depth estimation using tensorflow-lite.
 * The model used is [...] which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 * 
 * Pipeline:
 * multifilesrc -- jpegdec -- imxvideoconvert -- tee -----------------------------------------------------------------
 *                                                |                                                                  |
 *                                                |                                                             videomixer -- waylandsink
 *                                                |                                                                  |
 *                                                --- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"
#include "custom_depth_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <algorithm>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))

typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path videoPath;
  std::filesystem::path modelPath;
  std::string backend;
  std::string norm;
  PerformanceType perfType;
  std::string textColor;
  char* graphPath;
  int camWidth;
  int camHeight;
  int framerate;
} ParserOptions;


int cmdParser(int argc, char **argv, ParserOptions& options)
{
  int c;
  int optionIndex;
  std::string perfDisplay;
  std::string camParams;
  std::string temp;
  imx::Imx imx{};
  static struct option longOptions[] = {
    {"help",          no_argument,       0, 'h'},
    {"backend",       required_argument, 0, 'b'},
    {"normalization", required_argument, 0, 'n'},
    {"camera_device", required_argument, 0, 'c'},
    {"model_path",    required_argument, 0, 'p'},
    {"video_file",    required_argument, 0, 'f'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {"cam_params",    required_argument, 0, 'r'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:f:d::t:g:r:",
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
                  << " (none,centered,scaled,centeredScaled)" << std::endl

                  << std::setw(25) << std::left << "  -c, --camera_device"
                  << std::setw(25) << std::left
                  << "Use the selected camera device (/dev/video{number})"
                  << std::endl

                  << std::setw(25) << std::left << "  -p, --model_path"
                  << std::setw(25) << std::left
                  << "Use the selected model path" << std::endl

                  << std::setw(25) << std::left << "  -f, --video_file"
                  << std::setw(25) << std::left
                  << "Use the selected video file instead of camera source" << std::endl
                  
                  << std::setw(25) << std::left << "  -d, --display_perf"
                  << std::setw(25) << std::left
                  << "Display performances, can specify time or freq" << std::endl
                  
                  << std::setw(25) << std::left << "  -t, --text_color"
                  << std::setw(25) << std::left
                  << "Color of performances displayed,"
                  << " can choose between red, green, blue, and black (white by default)" << std::endl
                  
                  << std::setw(25) << std::left << "  -g, --graph_path"
                  << std::setw(25) << std::left
                  << "Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)" << std::endl

                  << std::setw(25) << std::left << "  -r, --cam_params"
                  << std::setw(25) << std::left
                  << "Use the selected camera resolution and framerate" << std::endl;
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

      case 'f':
        options.videoPath.assign(optarg);
        break;

      case 'd':
        if (OPTIONAL_ARGUMENT_IS_PRESENT)
            perfDisplay.assign(optarg);

        if (perfDisplay == "freq") {
          options.perfType = PerformanceType::frequency;
        } else if (perfDisplay == "time") {
          options.perfType = PerformanceType::temporal;
        } else {
          options.perfType = PerformanceType::all;
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

      case 'r':
        camParams.assign(optarg);
        if (std::count( camParams.begin(), camParams.end(), ',') != 2) {
          log_error("-r parameter needs the following argument: width,height,framerate\n");
          return 1;
        }
        options.camWidth = std::stoi(camParams.substr(0, camParams.find(",")));
        temp = camParams.substr(camParams.find(",")+1);
        options.camHeight = std::stoi(temp.substr(0, temp.find(",")));
        options.framerate = std::stoi(temp.substr(temp.find(",")+1));
        break;

      default:
        break;
    }
  }
  return 0;
}


int main(int argc, char **argv)
{
  // Initialize command line parser with default values
  ParserOptions options;
  options.backend = "NPU";
  options.norm = "none";
  options.perfType = PerformanceType::none;
  options.graphPath = getenv("HOME");
  options.camWidth = 640;
  options.camHeight = 480;
  options.framerate = 30;
  if (cmdParser(argc, argv, options))
    return 0;

  imx::Imx imx{};
  if (imx.isIMX95() && (options.backend == "NPU")) {
    log_error("Example can't run on NPU in i.MX95\n");
    return 0;
  }

  // Initialize pipeline object
  GstPipelineImx pipeline;

  bool UseCameraSource = options.videoPath.empty();

  if (UseCameraSource) {
    // Add camera to pipeline
    CameraOptions camOpt = {
      .cameraDevice   = options.camDevice,
      .gstName        = "cam_src",
      .width          = options.camWidth,
      .height         = options.camHeight,
      .horizontalFlip = false,
      .format         = "",
      .framerate      = options.framerate,
    };
    GstCameraImx camera(camOpt);
    camera.addCameraToPipeline(pipeline);
  } else {
    // Add video to pipeline
    GstVideoFileImx video(options.videoPath, false);
    video.addVideoToPipeline(pipeline);
  }

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
                      depthEstimation.getModelWidth(),
                      depthEstimation.getModelHeight(),
                      "GRAY8",
                      30);
  appsrc.addAppSrcToPipeline(displayPipeline);

  // Add video transform because display plugin doesn't support GRAY8 format
  GstVideoImx gstvideoimx;
  gstvideoimx.videoTransform(displayPipeline, "", -1, -1, false, false, true);

  // Display processed video
  GstVideoPostProcess postProcess;
  postProcess.display(displayPipeline, options.perfType, options.textColor);

  // Parse pipeline to GStreamer pipeline
  displayPipeline.parse(options.graphPath);
  pipeline.parse(options.graphPath);

  // Connect callback functions
  DecoderData boxesData;
  boxesData.appSrc = displayPipeline.getElement("appsrc_video");
  pipeline.connectToElementSignal(tensorSinkName, newDataCallback, "new-data", &boxesData);

  // Run GStreamer pipeline
  displayPipeline.run();
  pipeline.run();

  return 0;
}
