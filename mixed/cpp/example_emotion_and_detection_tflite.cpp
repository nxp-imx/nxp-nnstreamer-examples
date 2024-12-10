/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for emotion and object detection using tensorflow-lite.
 * The model used is emotion_uint8_float32.tflite and ultraface_slim_uint8_float32.tflite for emotion detection,
 * and ssdlite_mobilenet_v2_coco_no_postprocess.tflite for object detection,
 * which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb 
 * Pipeline:
 * pipeline 1: v4l2src -- tee -- imxvideoconvert -----------------------------------------------------------------
 *                |                                                                                     |
 *                |                                                                                    cairooverlay ---
 *                |                                                                                     |       |      |
 *                --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink   |      |
 *                |                                                                                             |      |
 *                --- appsink                                                                        ------------      |
 *                                                                                                   |                 |
 * pipeline 2: appsrc -- videocrop -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink             |
 *                                                                                                                     |
 *             filesrc -- tee ---------------------------------------------------------------------------              |
 *                |                                                                                     |              |
 *                |                                                                              video_compositor -- video_compositor -- waylandsink
 *                |                                                                                     |
 *                --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"
#include "custom_emotion_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <math.h>
#include <cassert>

// Check if command parser has an optional argument
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


const int MODEL_LATENCY_NS_CPU = 500000000;
const int MODEL_LATENCY_NS_GPU_VSI = 1000000000;
const int MODEL_LATENCY_NS_NPU_VSI = 25000000;


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path fPath;
  std::filesystem::path ePath;
  std::filesystem::path dPath;
  std::filesystem::path videoPath;
  std::string fBackend;
  std::string eBackend;
  std::string dBackend;
  std::string fNorm;
  std::string eNorm;
  std::string dNorm;
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
  std::string temp;
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
                  << "Use the selected video file" << std::endl
                  
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
        temp = backend.substr(backend.find(",")+1);
        options.eBackend = temp.substr(0, temp.find(","));
        options.dBackend = temp.substr(temp.find(",")+1);
        break;

      case 'n':
        modelNorm.assign(optarg);
        options.fNorm = modelNorm.substr(0, modelNorm.find(","));
        temp = modelNorm.substr(modelNorm.find(",")+1);
        options.eNorm = temp.substr(0, temp.find(","));
        options.dNorm = temp.substr(temp.find(",")+1);
        break;

      case 'c':
        options.camDevice.assign(optarg);
        break;

      case 'p':
        modelPath.assign(optarg);
        options.fPath = modelPath.substr(0, modelPath.find(","));
        temp = modelPath.substr(modelPath.find(",")+1);
        options.ePath = temp.substr(0, temp.find(","));
        options.dPath = temp.substr(temp.find(",")+1);
        break;

      case 'l': {
        dataDir.labelsDir.assign(optarg);

        std::string labels = dataDir.labelsDir.string();
        labels = labels.substr(0, dataDir.labelsDir.string().find(","));
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
  // Set command line parser with default values
  ParserOptions options;
  options.eBackend = "NPU";
  options.eNorm = "none";
  options.fBackend = "NPU";
  options.fNorm = "none";
  options.dBackend = "NPU";
  options.dNorm = "none";
  if (cmdParser(argc, argv, options))
    return 0;

  imx::Imx imx{};
  if ((imx.socId() == imx::IMX95) && (options.fBackend == "NPU")) {
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
                      640,
                      480,
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

  // Create pipeline object for face detection, and object detection
  GstPipelineImx pipeline;

  // Add camera to pipeline
  GstCameraImx camera(options.camDevice,
                      "cam_src",
                      CAMERA_INPUT_WIDTH,
                      CAMERA_INPUT_HEIGHT,
                      false,
                      "",
                      30);
  camera.addCameraToPipeline(pipeline);

  // Add another tee element for parallelization of tasks
  std::string teeName = "tvideo";
  pipeline.doInParallel(teeName);

  // Add a branch to tee element for face inference
  // and model post processing
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

  // Add a branch to tee element to process result of emotion detection
  GstQueueOptions imgQueue = {
    .queueName     = "thread-img",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, imgQueue);

  // Add text overlay and display it
  std::string cairoName = "cairooverlay";
  GstVideoPostProcess postProcess;
  gstvideoimx.videoTransform(pipeline, "RGB16", -1, -1, false);
  postProcess.addCairoOverlay(pipeline, cairoName);

  std::string compositor = "comp";
  pipeline.linkToVideoCompositor(compositor);

  // Add a branch to tee element to get video stream with appsink
  GstQueueOptions sinkQueue = {
    .queueName     = "thread-sink",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeName, sinkQueue);
  AppSinkOptions asOptions = {
    .name         = "appsink_video",
    .sync         = false,
    .maxBuffers   = 1,
    .drop         = true,
    .emitSignals  = true,
  };
  postProcess.addAppSink(pipeline, asOptions);

  // Add video to pipeline
  GstVideoFileImx PowerJump(options.videoPath, 640, 480);
  PowerJump.addVideoToPipeline(pipeline);

  // Add a tee element for parallelization of tasks
  std::string teeClassDet = "teeClassDet";
  pipeline.doInParallel(teeClassDet);

  // Add a branch to tee element for object detection inference
  // and model post processing
  GstQueueOptions nnDetQueue = {
    .queueName     = "thread-nn-det",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeClassDet, nnDetQueue);

  // Add object detection inference
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
    .outDim       = {PowerJump.getWidth(), PowerJump.getHeight()},
    .inDim        = {detection.getModelWidth(), detection.getModelHeight()},
    .trackResult  = false,
    .logResult    = false,
  };
  detDecoder.addBoundingBoxes(pipeline, decOptions);

  // Link decoder result to a video compositor
  std::string compositorName = "mix";
  pipeline.linkToVideoCompositor(compositorName);

  // Add a branch to tee element for inference and model post processing
  GstQueueOptions outputQueue = {
    .queueName     = "thread-out",
    .maxSizeBuffer = 1,
    .leakType      = GstQueueLeaky::downstream,
  };
  pipeline.addBranch(teeClassDet, outputQueue);

  // Set latency of model for video compositor
  int latency;
  if (options.dBackend == "NPU") {
    latency = MODEL_LATENCY_NS_NPU_VSI;
  } else if (options.dBackend == "GPU") {
    latency = MODEL_LATENCY_NS_GPU_VSI;
  } else {
    latency = MODEL_LATENCY_NS_CPU;
  }
  GstVideoImx gstvideosink{};
  gstvideosink.videoCompositor(pipeline, compositorName, latency);
  gstvideosink.videoCompositor(pipeline, compositor, 8*latency, displayPosition::split);
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, false);
  
  // Parse pipelines to GStreamer pipelines
  emotionPipeline.parse(argc, argv, options.graphPath);
  pipeline.parse(argc, argv, options.graphPath);

  // Connect callback functions to tensor sink of each pipeline, cairo overlay,
  // appsink, and appsrc to process inference output
  DecoderData boxesData;
  boxesData.appSrc = emotionPipeline.getElement("appsrc_video");
  boxesData.videocrop = emotionPipeline.getElement("video_crop");
  emotionPipeline.connectToElementSignal(tensorSinkEmo, secondaryNewDataCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(tensorSinkFace, newDataCallback, "new-data", &boxesData);
  pipeline.connectToElementSignal(cairoName, drawCallback, "draw", &boxesData);
  pipeline.connectToElementSignal("appsink_video", sinkCallback, "new-sample", &boxesData);

  // Run GStreamer pipelines
  emotionPipeline.run();
  pipeline.run();

  return 0;
}