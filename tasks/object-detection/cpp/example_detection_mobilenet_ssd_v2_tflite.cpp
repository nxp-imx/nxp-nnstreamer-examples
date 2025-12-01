/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for object detection using tensorflow-lite. 
 * The model used is ssdlite_mobilenet_v2_coco_no_postprocess.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * source --- tee -----------------------------------------------------------------------------------
 *             |                                                                                     |
 *             |                                                                              video_compositor -- waylandsink
 *             |                                                                                     |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"

#include <iostream>
#include <getopt.h>
#include <algorithm>

#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))
#define MODEL_LATENCY_NS_CPU          300000000
#define MODEL_LATENCY_NS_GPU_VSI      500000000
#define MODEL_LATENCY_NS_NPU_VSI      20000000
#define MODEL_LATENCY_NS_NPU_ETHOS    15000000
#define MODEL_LATENCY_NS_NPU_NEUTRON  20000000


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path videoPath;
  std::filesystem::path modelPath;
  std::string backend;
  std::string norm;
  DataDir dataDir;
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
    {"labels_path",   required_argument, 0, 'l'},
    {"boxes_path",    required_argument, 0, 'x'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {"cam_params",    required_argument, 0, 'r'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:f:l:x:d::t:g:r:",
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
    .outDim       = {pipeline.getDisplayWidth(), pipeline.getDisplayHeight()},
    .inDim        = {detection.getModelWidth(), detection.getModelHeight()},
    .trackResult  = false,
    .logResult    = false,
  };
  decoder.addBoundingBoxes(pipeline, decOptions);

  // Initialize compositor element
  std::string compositorName = "mix";
  GstVideoCompositorImx compositor(compositorName);

  // Add tensor decoder output to the compositor
  compositorInputParams firstInputParams = {
    .position     = displayPosition::center,
    .order        = 2,
    .keepRatio    = false,
    .transparency = true,
  };
  compositor.addToCompositor(pipeline, firstInputParams);

  // Add a branch to tee element to display result
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 2,
    .leakType      = (UseCameraSource) ? GstQueueLeaky::downstream : GstQueueLeaky::no,
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

  // Add camera input to the compositor
  compositorInputParams secondInputParams = {
    .position     = displayPosition::center,
    .order        = 1,
    .keepRatio    = false,
    .transparency = false,
  };
  compositor.addToCompositor(pipeline, secondInputParams);

  // Compositing pipeline input video with model processed output
  compositor.addCompositorToPipeline(pipeline, latency);

  // Display processed video
  GstVideoPostProcess postProcess;
  postProcess.display(pipeline, options.perfType, options.textColor);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}