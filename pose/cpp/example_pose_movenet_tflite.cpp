/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for pose detection using tensorflow-lite.
 * The model used is movenet_single_pose_lightning.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * filesrc -- videocrop -- tee -----------------------------------------------------------------------------------
 *                          |                                                                                     |
 *                          |                                                                               cairooverlay -- waylandsink
 *                          |                                                                                     |
 *                           --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink
 */

#include "common.hpp"
#include "custom_pose_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <math.h>
#include <cassert>
#include <algorithm>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path modelPath;
  std::filesystem::path videoPath;
  std::string backend;
  std::string norm;
  std::string useCamera;
  bool time;
  bool freq;
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
    {"use_camera",    required_argument, 0, 'u'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {"cam_params",    required_argument, 0, 'r'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:f:u:d::t:g:r:",
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

                  << std::setw(25) << std::left << "  -f, --video_file"
                  << std::setw(25) << std::left
                  << "Use the selected video file" << std::endl

                  << std::setw(25) << std::left << "  -u, --use_camera"
                  << std::setw(25) << std::left
                  << "If we use camera or video input" << std::endl
                  
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

      case 'u':
        options.useCamera.assign(optarg);
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
  options.norm = "castuInt8";
  options.useCamera = "0";
  options.time = false;
  options.freq = false;
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

  if ((options.useCamera == "1") || imx.isIMX93() || imx.isIMX95()) {
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
    GstVideoFileImx PowerJumpVideo(options.videoPath);
    PowerJumpVideo.addVideoToPipeline(pipeline);
  }

  // Video pre processing
  GstVideoImx gstvideoimx;
  int inputDim = std::min(options.camWidth, options.camHeight);
  gstvideoimx.videocrop(pipeline,
                        "crop",
                        inputDim,
                        inputDim,
                        -1,
                        -1,
                        -1,
                        -1);

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
  TFliteModelInfos pose(options.modelPath, options.backend, options.norm);
  pose.addInferenceToPipeline(pipeline, "pose_filter");

  // Get inference output for custom processing
  std::string tensorSinkName = "tensor_sink";
  pipeline.addTensorSink(tensorSinkName);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 2,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Add text overlay and display result
  std::string overlayName = "cairo";
  GstVideoPostProcess postProcess;
  postProcess.addCairoOverlay(pipeline, overlayName);

  float scaleFactor = 15.0f/640; // Default font size is 15 pixels for a width of 640
  pipeline.enablePerfDisplay(options.freq, options.time, options.camWidth * scaleFactor, options.textColor);
  postProcess.display(pipeline, true);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);
  
  // Connect callback functions to tensor sink and cairo overlay,
  // to process inference output
  DecoderData kptsData;
  kptsData.inputDim = inputDim;
  pipeline.connectToElementSignal(tensorSinkName, newDataCallback, "new-data", &kptsData);
  pipeline.connectToElementSignal(overlayName, drawCallback, "draw", &kptsData);

  // Run GStreamer pipeline
  pipeline.run();
  
  return 0;
}
