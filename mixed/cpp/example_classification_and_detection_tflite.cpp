/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for image classification and object detection using tensorflow-lite. 
 * The model used is mobilenet_v1_1.0_224.tflite for image classification, and ssdlite_mobilenet_v2_coco_no_postprocess.tflite for object detection,
 * which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:           
 * v4l2src -- tee -----------------------------------------------------------------------------------
 *             |                                                                                     |
 *             |                                                                              video_compositor -- textoverlay -- tee -- autovideosink
 *             |                                                                                     |                 |          |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder          |          --- vpuenc_h264 -- h264parse -- qtmux / matroskamux -- filesink
 *             |                                                                                                       |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder --------- 
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
const int MODEL_LATENCY_NS_CPU = 500000000;
const int MODEL_LATENCY_NS_GPU_VSI = 1000000000;
const int MODEL_LATENCY_NS_NPU_VSI = 25000000;


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path cPath;
  std::filesystem::path dPath;
  std::filesystem::path videoPath;
  std::string cBackend;
  std::string dBackend;
  std::string cNorm;
  std::string dNorm;
  DataDir cDataDir;
  DataDir dDataDir;
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
  DataDir dataDir;
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
    {"video_file",    required_argument, 0, 'f'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:c:p:l:x:f:d::t:g:",
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

                  << std::setw(25) << std::left << "  -f, --video_file"
                  << std::setw(25) << std::left
                  << "Use the selected path for generated video" << std::endl
                  
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
        options.cBackend = backend.substr(0, backend.find(","));
        options.dBackend = backend.substr(backend.find(",")+1);
        break;

      case 'n':
        modelNorm.assign(optarg);
        options.cNorm = modelNorm.substr(0, modelNorm.find(","));
        options.dNorm = modelNorm.substr(modelNorm.find(",")+1);
        break;

      case 'c':
        options.camDevice.assign(optarg);
        break;

      case 'p':
        modelPath.assign(optarg);
        options.cPath = modelPath.substr(0, modelPath.find(","));
        options.dPath = modelPath.substr(modelPath.find(",")+1);
        break;

      case 'l': {
        dataDir.labelsDir.assign(optarg);

        std::string labels = dataDir.labelsDir.string();
        labels = labels.substr(0, dataDir.labelsDir.string().find(","));
        options.cDataDir.labelsDir = labels;

        labels = dataDir.labelsDir.string();
        labels = labels.substr(dataDir.labelsDir.string().find(",")+1);
        options.dDataDir.labelsDir = labels;
        break;
      }
      case 'x':
        dataDir.boxesDir.assign(optarg);
        options.dDataDir.boxesDir = dataDir.boxesDir;
        break;

      case 'f':
        options.videoPath.assign(optarg);
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
  options.cBackend = "NPU";
  options.dBackend = "NPU";
  options.cNorm = "none";
  options.dNorm = "none";
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

  // Add a branch to tee element for classification inference 
  // and model post processing
  GstQueueOptions nnClassQueue = {
    .queueName     = "thread-nn-class",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnClassQueue);

  // Add classification inference
  TFliteModelInfos classification(options.cPath,
                                  options.cBackend,
                                  options.cNorm);
  classification.addInferenceToPipeline(pipeline, "classification_filter");
  
  // Add NNStreamer inference output decoding
  NNDecoder cDecoder;
  cDecoder.addImageLabeling(pipeline, options.cDataDir.labelsDir.string());

  // Link decoder result to a text overlay
  std::string overlayName = "overlay";
  pipeline.linkToTextOverlay(overlayName);

  // Add a branch to tee element for classification inference 
  // and model post processing
  GstQueueOptions nnDetQueue = {
    .queueName     = "thread-nn-det",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, nnDetQueue);

  // Add detection inference
  TFliteModelInfos detection(options.dPath, options.dBackend, options.dNorm);
  detection.addInferenceToPipeline(pipeline, "detection_filter");

  // Add NNStreamer inference output decoding
  NNDecoder detDecoder;
  
  SSDMobileNetCustomOptions opt3 = { 
    .boxesPath = options.dDataDir.boxesDir.string(),
  };

  BoundingBoxesOptions decOptions = {
    .modelName    = ModeBoundingBoxes::mobilenetssd,
    .labelsPath   = options.dDataDir.labelsDir.string(),
    .option3      = setCustomOptions(opt3),
    .outDim       = {camera.getWidth(), camera.getHeight()},
    .inDim        = {detection.getModelWidth(), detection.getModelHeight()},
    .trackResult  = false,
    .logResult    = false,
  };
  detDecoder.addBoundingBoxes(pipeline, decOptions);

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
  if (options.dBackend == "NPU") {
    latency = MODEL_LATENCY_NS_NPU_VSI;
  } else if (options.dBackend == "GPU") {
    latency = MODEL_LATENCY_NS_GPU_VSI;
  } else {
    latency = MODEL_LATENCY_NS_CPU;
  }

  // Add video compositor
  GstVideoImx gstvideosink{};
  gstvideosink.videoCompositor(pipeline, compositorName, latency);

  // Add text overlay
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

  imx::Imx imx{};
  if (!imx.isIMX9()) {
    // Add another tee element
    std::string ppTeeName = "save";
    pipeline.doInParallel(ppTeeName);

    // Add a branch to tee element to save output to MKV video
    std::string videoFormat = "mkv";
    GstQueueOptions saveQueue = {
      .queueName = "thread-save",
    };
    pipeline.addBranch(ppTeeName, saveQueue);
    postProcess.saveToVideo(pipeline, videoFormat, options.videoPath);

    // Add a branch to tee element to display result
    GstQueueOptions displayQueue = {
      .queueName = "thread-display",
    };
    pipeline.addBranch(ppTeeName, displayQueue);
  }
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, false);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}
