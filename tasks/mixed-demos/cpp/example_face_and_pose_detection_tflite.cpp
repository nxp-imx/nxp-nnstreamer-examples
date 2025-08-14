/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for face and pose detection using tensorflow-lite.
 * The model used is ultraface_slim_uint8_float32.tflite for face detection, and movenet_single_pose_lightning.tflite for pose detection,
 * which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * v4l2src -- videocrop -- tee -----------------------------------------------------------------------------------
 *                          |                                                                                     |
 *                          |                                                                               cairooverlay -- cairooverlay -- waylandsink
 *                          |                                                                                     |               |
 *                          --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink           |
 *                          |                                                                                                     |
 *                          --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink -----------
 */

#include "common.hpp"
#include "custom_face_and_pose_decoder.hpp"

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
  std::filesystem::path fPath;
  std::filesystem::path pPath;
  std::string fBackend;
  std::string pBackend;
  std::string fNorm;
  std::string pNorm;
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
  std::string backend;
  std::string modelNorm;
  std::string modelPath;
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
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {"cam_params",    required_argument, 0, 'r'},
    {0,               0,                 0,   0}
  };
  
  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:d::t:g:r:",
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
        options.fBackend = backend.substr(0, backend.find(","));
        options.pBackend = backend.substr(backend.find(",")+1);
        break;

      case 'n':
        modelNorm.assign(optarg);
        options.fNorm = modelNorm.substr(0, modelNorm.find(","));
        options.pNorm = modelNorm.substr(modelNorm.find(",")+1);
        break;

      case 'c':
        options.camDevice.assign(optarg);
        break;

      case 'p':
        modelPath.assign(optarg);
        options.fPath = modelPath.substr(0, modelPath.find(","));
        options.pPath = modelPath.substr(modelPath.find(",")+1);
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
  options.fBackend = "NPU";
  options.pBackend = "NPU";
  options.fNorm = "none";
  options.pNorm = "none";
  options.perfType = PerformanceType::none;
  options.graphPath = getenv("HOME");
  options.camWidth = 640;
  options.camHeight = 480;
  options.framerate = 30;
  if (cmdParser(argc, argv, options))
    return 0;

  imx::Imx imx{};
  if (options.pBackend == "NPU") {
    if (imx.isIMX95() || imx.isIMX93()) {
      log_error("Example can't run on NPU in %s\n", imx.socName().c_str());
      return 0;
    }
  }

  // Initialize pipeline object
  GstPipelineImx pipeline;

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

  // Video pre processing
  GstVideoImx gstvideoimx;
  int inputDim = std::min(options.camWidth, options.camHeight);
  gstvideoimx.videocrop(pipeline,
                        "crop",
                        inputDim,
                        inputDim);

  // Add a tee element for parallelization of tasks
  std::string teeName = "t";
  pipeline.doInParallel(teeName);

  // Add a branch to tee element for face detection inference
  // and model post processing
  GstQueueOptions nnFaceQueue = {
    .queueName     = "thread-nn-face",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnFaceQueue);

  // Add face detection inference
  int numThreads;
  if ((options.fBackend == "CPU") && (options.pBackend == "CPU"))
    numThreads = std::thread::hardware_concurrency()/2;
  else
    numThreads = std::thread::hardware_concurrency();
  TFliteModelInfos faceDetection(options.fPath, options.fBackend, options.fNorm, numThreads);
  faceDetection.addInferenceToPipeline(pipeline, "face_filter");

  // Get inference output for custom processing
  std::string tsinkFace = "tsink_fd";
  pipeline.addTensorSink(tsinkFace);

  // Add a branch to tee element for pose detection inference
  // and model post processing
  GstQueueOptions nnPoseQueue = {
    .queueName     = "thread-nn-pose",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnPoseQueue);

  // Add pose detection inference
  TFliteModelInfos pose(options.pPath, options.pBackend, options.pNorm, numThreads);
  pose.addInferenceToPipeline(pipeline, "pose_filter");

  // Get inference output for custom processing
  std::string tsinkPose = "tsink_pd";
  pipeline.addTensorSink(tsinkPose);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Add text overlay and display it
  std::string overlayFace = "cairoFace";
  GstVideoPostProcess postProcess;
  postProcess.addCairoOverlay(pipeline, overlayFace);

  std::string overlayPose = "cairoPose";
  postProcess.addCairoOverlay(pipeline, overlayPose);

  postProcess.display(pipeline, options.perfType, options.textColor);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(options.graphPath);

  // Connect callback functions to tensor sink and cairo overlay elements,
  // to process inferences output
  FaceData boxesData;
  boxesData.inputDim = inputDim;
  pipeline.connectToElementSignal(tsinkFace, newDataFaceCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(overlayFace, drawFaceCallback, "draw", &boxesData);

  PoseData kptsData;
  kptsData.inputDim = inputDim;
  pipeline.connectToElementSignal(tsinkPose, newDataPoseCallback, "new-data", &kptsData);
  pipeline.connectToElementSignal(overlayPose, drawPoseCallback, "draw", &kptsData);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}
