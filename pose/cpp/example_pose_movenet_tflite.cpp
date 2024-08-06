/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * NNstreamer application for pose detection using tensorflow-lite.
 * The model used is movenet_single_pose_lightning.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * filesrc -- videocrop -- tee -----------------------------------------------------------------------------------
 *                          |                                                                                     |
 *                          |                                                                               cairooverlay -- autovideosink
 *                          |                                                                                     |                                                                                   |
 *                           --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_sink
 */

#include "common.hpp"
#include "custom_pose_decoder.hpp"

#include <iostream>
#include <getopt.h>
#include <math.h>
#include <cassert>

// Check if command parser has an optional argument
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


const int INPUT_WIDTH = 480;
const int INPUT_HEIGHT = 480;


typedef struct {
  std::filesystem::path camDevice;
  std::filesystem::path modelPath;
  std::filesystem::path videoPath;
  std::string backend;
  std::string norm;
  std::string useCamera;
  bool time = false;
  bool freq = false;
  std::string textColor;
} ParserOptions;


int cmdParser(int argc, char **argv, ParserOptions& options)
{
  int c;
  int optionIndex;
  std::string perfDisplay;
  static struct option longOptions[] = {
    {"help",          no_argument,       0, 'h'},
    {"backend",       required_argument, 0, 'b'},
    {"normalization", required_argument, 0, 'n'},
    {"model_path",    required_argument, 0, 'p'},
    {"video_file",    required_argument, 0, 'f'},
    {"use_camera",    required_argument, 0, 'u'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:p:f:u:d::t:",
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
                  << " can choose between red, green, blue, and black (white by default)" << std::endl;
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
  options.backend = "CPU";
  options.norm = "castInt32";
  options.useCamera = "0";
  if (cmdParser(argc, argv, options))
    return 0;

  imx::Imx imx{};
  if ((options.useCamera == "1") || (imx.isIMX9())) {
    // Add camera to pipeline
    GstCameraImx camera(options.camDevice,
                        "cam_src",
                        INPUT_WIDTH,
                        INPUT_HEIGHT,
                        false);
    camera.addCameraToPipeline(pipeline);
  } else {
    // Add video to pipeline
    GstVideoFileImx PowerJumpVideo(options.videoPath);
    PowerJumpVideo.addVideoToPipeline(pipeline);
  }

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
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv);
  
  // Connect callback functions to tensor sink and cairo overlay,
  // to process inference output
  DecoderData kptsData;
  pipeline.connectToElementSignal(tensorSinkName, newDataCallback, "new-data", &kptsData);
  pipeline.connectToElementSignal(overlayName, drawCallback, "draw", &kptsData);

  // Run GStreamer pipeline
  pipeline.run();
  
  return 0;
}
