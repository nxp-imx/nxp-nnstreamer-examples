/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for image classification using tensorflow-lite on two cameras. 
 * The model used is mobilenet_v1_1.0_224.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * v4l2src -- tee -----------------------------------------------------------------------------------
 *             |                                                                                     |
 *             |                                                                                textoverlay ----------
 *             |                                                                                     |                |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder         |
 *                                                                                                                    |
 * v4l2src -- tee -----------------------------------------------------------------------------------                 |
 *             |                                                                                     |                |
 *             |                                                                                textoverlay -- video_compositor -- waylandsink
 *             |                                                                                     |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 * 
 */

#include "common.hpp"

#include <iostream>
#include <getopt.h>
#include <algorithm>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


typedef struct {
  std::filesystem::path camDevice1;
  std::filesystem::path camDevice2;
  std::filesystem::path modelPathCam1;
  std::filesystem::path modelPathCam2;
  std::string backendCam1;
  std::string backendCam2;
  std::string normCam1;
  std::string normCam2;
  DataDir dataDir;
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
  std::string camDevice;
  std::string modelPath;
  std::string backend;
  std::string norm;
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
    {"labels_path",   required_argument, 0, 'l'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {"cam_params",    required_argument, 0, 'r'},
    {0,               0,                 0,   0}
  };
  
  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:l:d::t:g:r:",
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
                  << "Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)" << std::endl

                  << std::setw(25) << std::left << "  -r, --cam_params"
                  << std::setw(25) << std::left
                  << "Use the selected camera resolution and framerate" << std::endl;
        return 1;

      case 'b':
        backend.assign(optarg);
        options.backendCam1 = backend.substr(0, backend.find(","));
        options.backendCam2 = backend.substr(backend.find(",")+1);
        break;

      case 'n':
        norm.assign(optarg);
        options.normCam1 = norm.substr(0, norm.find(","));
        options.normCam2 = norm.substr(norm.find(",")+1);
        break;

      case 'c':
        camDevice.assign(optarg);
        options.camDevice1 = camDevice.substr(0, camDevice.find(","));
        options.camDevice2 = camDevice.substr(camDevice.find(",")+1);
        break;

      case 'p':
        modelPath.assign(optarg);
        options.modelPathCam1 = modelPath.substr(0, modelPath.find(","));
        options.modelPathCam2 = modelPath.substr(modelPath.find(",")+1);
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
  options.backendCam1 = "NPU";
  options.backendCam2 = "NPU";
  options.normCam1 = "none";
  options.normCam2 = "none";
  options.time = false;
  options.freq = false;
  options.graphPath = getenv("HOME");
  options.camWidth = 640;
  options.camHeight = 480;
  options.framerate = 30;
  if (cmdParser(argc, argv, options))
    return 0;

  // Initialize pipeline object
  GstPipelineImx pipeline;

  // Add first camera to pipeline
  CameraOptions camOpt = {
    .cameraDevice   = options.camDevice1,
    .gstName        = "cam_src",
    .width          = options.camWidth,
    .height         = options.camHeight,
    .horizontalFlip = false,
    .format         = "",
    .framerate      = options.framerate,
  };
  GstCameraImx camera(camOpt);
  camera.addCameraToPipeline(pipeline);

  // Add a tee element for parallelization of tasks for the first camera
  std::string firstCamTee = "firstCam";
  pipeline.doInParallel(firstCamTee);

  // Add a branch to tee element for first camera inference
  GstQueueOptions nnQueue = {
    .queueName     = "first-cam-inference",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(firstCamTee, nnQueue);

  // Add model inference
  TFliteModelInfos firstModel(options.modelPathCam1, options.backendCam1, options.normCam1);
  firstModel.addInferenceToPipeline(pipeline, "cam1");

  // Add NNStreamer inference output decoding
  NNDecoder decoder;
  decoder.addImageLabeling(pipeline, options.dataDir.labelsDir.string());

  // Link decoder result to a text overlay
  std::string overlay = "overlay";
  pipeline.linkToTextOverlay(overlay);

  // Add a branch to tee element for first camera overlay
  GstQueueOptions overlayQueue = {
    .queueName     = "first-cam-overlay",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(firstCamTee, overlayQueue);

  // Add a text overlay to first camera video stream
  GstVideoPostProcess postProcess;
  TextOverlayOptions firstOverlayOpt = {
    .gstName    = overlay,
    .fontName   = "Sans",
    .fontSize   = 24,
    .color      = "",
    .vAlignment = "baseline",
    .hAlignment = "center",
    .text       = "", // set only if linkToTextOverlay is not used
  };
  postProcess.addTextOverlay(pipeline, firstOverlayOpt);

  std::string compositorName = "mix";
  pipeline.linkToVideoCompositor(compositorName);

  // Add second camera to pipeline
  CameraOptions camOpt2 = {
    .cameraDevice   = options.camDevice2,
    .gstName        = "cam_src2",
    .width          = options.camWidth,
    .height         = options.camHeight,
    .horizontalFlip = false,
    .format         = "",
    .framerate      = options.framerate,
  };
  GstCameraImx camera2(camOpt2);
  camera2.addCameraToPipeline(pipeline);

  // Add a tee element for parallelization of tasks for the second camera
  std::string secondCamTee = "secondCam";
  pipeline.doInParallel(secondCamTee);

  // Add a branch to tee element for second camera inference
  GstQueueOptions nn2Queue = {
    .queueName     = "second-cam-inference",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(secondCamTee, nn2Queue);

  // Add model inference
  TFliteModelInfos secondModel(options.modelPathCam2, options.backendCam2, options.normCam2);
  secondModel.addInferenceToPipeline(pipeline, "cam2");

  // Add NNStreamer inference output decoding
  decoder.addImageLabeling(pipeline, options.dataDir.labelsDir.string());

  // Link decoder result to a text overlay
  std::string overlay2 = "overlay2";
  pipeline.linkToTextOverlay(overlay2);

  // Add a branch to tee element for second camera overlay,
  // and display of first and second camera output
  GstQueueOptions overlay2Queue = {
    .queueName     = "second-cam-overlay",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(secondCamTee, overlay2Queue);

  TextOverlayOptions secondOverlayOpt = {
    .gstName    = overlay2,
    .fontName   = "Sans",
    .fontSize   = 24,
    .color      = "",
    .vAlignment = "baseline",
    .hAlignment = "center",
    .text       = "", // set only if linkToTextOverlay is not used
  };

  postProcess.addTextOverlay(pipeline, secondOverlayOpt);

  // Add video compositor to display both cameras
  GstVideoImx gstvideoimx{};
  int modelLatency = 10000000;
  gstvideoimx.videoCompositor(pipeline, compositorName, modelLatency, displayPosition::split);

  float scaleFactor = 15.0f/640; // Default font size is 15 pixels for a width of 640
  pipeline.enablePerfDisplay(options.freq, options.time, options.camWidth * scaleFactor, options.textColor);
  postProcess.display(pipeline);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}
