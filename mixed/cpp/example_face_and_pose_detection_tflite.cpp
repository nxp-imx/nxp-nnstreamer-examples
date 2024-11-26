/**
 * Copyright 2024 NXP
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
 *                          |                                                                               cairooverlay -- cairooverlay -- autovideosink
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

// Check if command parser has an optional argument
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


const int CAMERA_INPUT_WIDTH = 640;
const int CAMERA_INPUT_HEIGHT = 480;


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path fPath;
  std::filesystem::path pPath;
  std::string fBackend;
  std::string pBackend;
  std::string fNorm;
  std::string pNorm;
  bool time = false;
  bool freq = false;
  std::string textColor;
  char* graphPath = getenv("HOME");
} ParserOptions;


int cmdParser(int argc, char **argv, ParserOptions& options)
{
  int c;
  int optionIndex;
  std::string backend;
  std::string modelNorm;
  std::string modelPath;
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
  options.fBackend = "NPU";
  options.pBackend = "NPU";
  options.fNorm = "none";
  options.pNorm = "castuInt8";
  if (cmdParser(argc, argv, options))
    return 0;

  imx::Imx imx{};
  if ((imx.socId() == imx::IMX95) && (options.fBackend == "NPU")) {
    log_error("Example can't run on NPU in i.MX95\n");
    return 0;
  }

  // Add camera to pipeline
  GstCameraImx camera(options.camDevice,
                      "cam_src",
                      CAMERA_INPUT_WIDTH,
                      CAMERA_INPUT_HEIGHT,
                      false);
  camera.addCameraToPipeline(pipeline);

  // Video pre processing
  GstVideoImx gstvideoimx;
  gstvideoimx.videocrop(pipeline,
                        "crop",
                        INPUT_WIDTH,
                        INPUT_HEIGHT,
                        -1,
                        -1,
                        -1,
                        -1);

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
  TFliteModelInfos faceDetection(options.fPath, options.fBackend, options.fNorm);
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
  TFliteModelInfos pose(options.pPath, options.pBackend, options.pNorm);
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
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, false);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Connect callback functions to tensor sink and cairo overlay elements,
  // to process inferences output
  FaceData boxesData;
  pipeline.connectToElementSignal(tsinkFace, newDataFaceCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(overlayFace, drawFaceCallback, "draw", &boxesData);

  PoseData kptsData;
  pipeline.connectToElementSignal(tsinkPose, newDataPoseCallback, "new-data", &kptsData);
  pipeline.connectToElementSignal(overlayPose, drawPoseCallback, "draw", &kptsData);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}
