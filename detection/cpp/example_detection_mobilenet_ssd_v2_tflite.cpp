/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for object detection using tensorflow-lite. 
 * The model used is ssdlite_mobilenet_v2_coco_no_postprocess.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * v4l2src -- tee -----------------------------------------------------------------------------------
 *             |                                                                                     |
 *             |                                                                              video_compositor -- waylandsink
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
const int MODEL_LATENCY_NS_CPU = 300000000;
const int MODEL_LATENCY_NS_GPU_VSI = 500000000;
const int MODEL_LATENCY_NS_NPU_VSI = 20000000;
const int MODEL_LATENCY_NS_NPU_ETHOS = 15000000;
const int MODEL_LATENCY_NS_NPU_NEUTRON = 20000000;


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
    {"boxes_path",    required_argument, 0, 'x'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:l:x:d::t:g:",
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

                  << std::setw(25) << std::left << "  -x, --boxes_path"
                  << std::setw(25) << std::left
                  << "Use the selected boxes path" << std::endl

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

      case 'x':
        options.dataDir.boxesDir.assign(optarg);
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
  TFliteModelInfos detection(options.modelPath, options.backend, options.norm);
  detection.addInferenceToPipeline(pipeline, "detection_filter");

  // Add NNStreamer inference output decoding
  NNDecoder decoder;

  // Custom options specific to the model
  SSDMobileNetCustomOptions customOptions = {
    .boxesPath = options.dataDir.boxesDir.string(),
  };

  BoundingBoxesOptions decOptions = {
    .modelName    = ModeBoundingBoxes::mobilenetssd,
    .labelsPath   = options.dataDir.labelsDir.string(),
    .option3      = setCustomOptions(customOptions),
    .outDim       = {camera.getWidth(), camera.getHeight()},
    .inDim        = {detection.getModelWidth(), detection.getModelHeight()},
    .trackResult  = false,
    .logResult    = false,
  };
  decoder.addBoundingBoxes(pipeline, decOptions);

  // Link decoder result to a video compositor
  std::string compositorName = "mix";
  pipeline.linkToVideoCompositor(compositorName);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Set latency of model for video compositor
  int latency;
  imx::Imx imx{};
  if (options.backend == "NPU") {
    if (imx.isIMX8())
      latency = MODEL_LATENCY_NS_NPU_VSI;
    else if (imx.hasEthosNPU())
      latency = MODEL_LATENCY_NS_NPU_ETHOS;
    else
      latency = MODEL_LATENCY_NS_NPU_NEUTRON;
  } else if (imx.isIMX8() && (options.backend == "GPU")) {
    latency = MODEL_LATENCY_NS_GPU_VSI;
  } else {
    latency = MODEL_LATENCY_NS_CPU;
  }

  // Add video compositor
  GstVideoImx gstvideoimx{};
  gstvideoimx.videoCompositor(pipeline, compositorName, latency);

  // Display processed video
  GstVideoPostProcess postProcess;
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, false);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}