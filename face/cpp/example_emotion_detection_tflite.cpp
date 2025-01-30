/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for emotion detection using tensorflow-lite.
 * The model used is emotion_uint8_float32.tflite for emotion detection, and ultraface_slim_uint8_float32.tflite for face detection,
 * which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 * Pipeline:
 * pipeline 1: v4l2src -- tee -- imxvideoconvert --------------------------------------------------------
 *                |                                                                                     |
 *                |                                                                                    cairooverlay -- waylandsink
 *                |                                                                                     |       |
 *                --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink   |
 *                |                                                                                             |
 *                 --- appsink                                                                                  |
 *                                                                                                   ------------
 *                                                                                                   |
 * pipeline 2: appsrc -- videocrop -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink
 */

#include "common.hpp"
#include "custom_emotion_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <algorithm>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path fPath;
  std::filesystem::path ePath;
  std::string fBackend;
  std::string eBackend;
  std::string fNorm;
  std::string eNorm;
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
                  << "Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)" << std::endl

                  << std::setw(25) << std::left << "  -r, --cam_params"
                  << std::setw(25) << std::left
                  << "Use the selected camera resolution and framerate" << std::endl;
        return 1;

      case 'b':
        backend.assign(optarg);
        options.fBackend = backend.substr(0, backend.find(","));
        options.eBackend = backend.substr(backend.find(",")+1);
        break;

      case 'n':
        modelNorm.assign(optarg);
        options.fNorm = modelNorm.substr(0, modelNorm.find(","));
        options.eNorm = modelNorm.substr(modelNorm.find(",")+1);
        break;

      case 'c':
        options.camDevice.assign(optarg);
        break;

      case 'p':
        modelPath.assign(optarg);
        options.fPath = modelPath.substr(0, modelPath.find(","));
        options.ePath = modelPath.substr(modelPath.find(",")+1);
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

/**
 * This example uses 2 pipelines : one pipeline is used to detect faces,
 * while the second pipeline retrieves faces from the first pipeline, 
 * and apply emotion detection to it.
 */
int main(int argc, char **argv)
{
  // Initialize command line parser with default values
  ParserOptions options;
  options.eBackend = "NPU";
  options.eNorm = "none";
  options.fBackend = "NPU";
  options.fNorm = "none";
  options.time = false;
  options.freq = false;
  options.graphPath = getenv("HOME");
  options.camWidth = 640;
  options.camHeight = 480;
  options.framerate = 30;
  if (cmdParser(argc, argv, options))
    return 0;
  
  imx::Imx imx{};
  if (imx.isIMX95() && (options.fBackend == "NPU")) {
    log_error("Example can't run on NPU in i.MX95\n");
    return 0;
  }

  // Create a pipeline object for emotion detection inference
  GstPipelineImx emotionPipeline;

  // Add appsrc element to retrieve the video stream
  GstAppSrcImx appsrc("appsrc_video",
                      true,
                      false,
                      1,
                      GstQueueLeaky::downstream,
                      3,
                      options.camWidth,
                      options.camHeight,
                      "YUY2");
  appsrc.addAppSrcToPipeline(emotionPipeline);

  // The video stream is cropped to get a face
  GstVideoImx gstvideoimx {};
  gstvideoimx.videocrop(emotionPipeline, "video_crop", -1, -1);

  // Add model inference to get the emotion of a face
  TFliteModelInfos emotionDetection(options.ePath, options.eBackend, options.eNorm);
  emotionDetection.addInferenceToPipeline(emotionPipeline, "emotion_filter", "GRAY8");

  // Get inference output for custom processing
  std::string tensorSinkEmo = "tsink_fr";
  emotionPipeline.addTensorSink(tensorSinkEmo, false);

  // Create pipeline object for face detection
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

  // Add a tee element for parallelization of tasks
  std::string teeName = "tvideo";
  pipeline.doInParallel(teeName);

  // Add a branch to tee element for inference and model post processing
  GstQueueOptions nnQueue = {
    .queueName     = "thread-nn",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnQueue);

  // Add model inference
  TFliteModelInfos faceDetection(options.fPath, options.fBackend, options.fNorm);
  faceDetection.addInferenceToPipeline(pipeline, "face_filter");

  // Get inference output for custom processing
  std::string tensorSinkFace = "tsink_fd";
  pipeline.addTensorSink(tensorSinkFace);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Add text overlay and display result
  std::string overlayName = "cairooverlay";
  GstVideoPostProcess postProcess;
  gstvideoimx.videoTransform(pipeline, "RGB16", -1, -1, false);
  postProcess.addCairoOverlay(pipeline, overlayName);
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, false);

  // Add a branch to tee element to get video stream with appsink
  GstQueueOptions sinkQueue = {
    .queueName     = "thread-sink",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, sinkQueue);
  AppSinkOptions asOptions = {
    .gstName      = "appsink_video",
    .sync         = false,
    .maxBuffers   = 1,
    .drop         = true,
    .emitSignals  = true,
  };
  postProcess.addAppSink(pipeline, asOptions);

  // Parse pipelines to GStreamer pipelines
  emotionPipeline.parse(argc, argv, options.graphPath);
  pipeline.parse(argc, argv, options.graphPath);

  // Connect callback functions to tensor sink of each pipeline, cairo overlay,
  // appsink, and appsrc to process inference output
  DecoderData boxesData;
  boxesData.camWidth = options.camWidth;
  boxesData.camHeight = options.camHeight;
  boxesData.appSrc = emotionPipeline.getElement("appsrc_video");
  boxesData.videocrop = emotionPipeline.getElement("video_crop");
  emotionPipeline.connectToElementSignal(tensorSinkEmo, secondaryNewDataCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(tensorSinkFace, newDataCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(overlayName, drawCallback, "draw", &boxesData);
  pipeline.connectToElementSignal("appsink_video", sinkCallback, "new-sample", &boxesData);

  // Run GStreamer pipelines
  emotionPipeline.run();
  pipeline.run();

  return 0;
}