/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for segmentation using tensorflow-lite. 
 * The model used is deeplabv3_mnv2_dm05_pascal.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * multifilesrc -- jpegdec -- imxvideoconvert -- tee -----------------------------------------------------------------
 *                                                |                                                                  |
 *                                                |                                                             videomixer -- autovideosink
 *                                                |                                                                  |
 *                                                --- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include "common.hpp"

#include <iostream>
#include <getopt.h>

// Check if command parser has an optional argument
#define OPTIONAL_ARGUMENT_IS_PRESENT \
    ((optarg == NULL && optind < argc && argv[optind][0] != '-') \
     ? (bool) (optarg = argv[optind++]) \
     : (optarg != NULL))


typedef struct {
  std::filesystem::path modelPath;
  std::filesystem::path slideshowPath;
  std::string backend;
  std::string norm;
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
    {"model_path",    required_argument, 0, 'p'},
    {"images_file",   required_argument, 0, 'f'},
    {"display_perf",  optional_argument, 0, 'd'},
    {"text_color",    required_argument, 0, 't'},
    {"graph_path",    required_argument, 0, 'g'},
    {0,               0,                 0,   0}
  };

  while ((c = getopt_long(argc,
                          argv,
                          "hb:n:p:l:f:d::t:g:",
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

                  << std::setw(25) << std::left << "  -p, --model_path"
                  << std::setw(25) << std::left
                  << "Use the selected model path" << std::endl

                  << std::setw(25) << std::left << "  -f, --images_file"
                  << std::setw(25) << std::left
                  << "Use the selected images path" << std::endl
                  
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

      case 'p':
        options.modelPath.assign(optarg);
        break;

      case 'f':
        options.slideshowPath.assign(optarg);
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

  imx::Imx imx{};
  if ((imx.socId() == imx::IMX95) && (options.backend == "NPU")) {
    log_error("Example can't run on NPU in i.MX95\n");
    return 0;
  }

  // Add slideshow to pipeline
  GstSlideshowImx slideshow(options.slideshowPath);
  slideshow.addSlideshowToPipeline(pipeline);

  // Create model object to get it's size for cropping
  TFliteModelInfos segmentation(options.modelPath,
                                options.backend,
                                options.norm);
  GstVideoImx gstvideoimx{};
  gstvideoimx.videoTransform(pipeline,
                             "",
                             segmentation.getModelWidth(),
                             segmentation.getModelHeight(),
                             false);

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
  segmentation.addInferenceToPipeline(pipeline, "seg_filter");

  // Add NNStreamer inference output decoding
  NNDecoder decoder;
  ImageSegmentOptions decOptions = {
    .modelName = ModeImageSegment::tfliteDeeplab,
  };
  decoder.addImageSegment(pipeline, decOptions);

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

  // Add video compositor, we use videomixer because of height and weight
  // constraint on video compositor
  pipeline.addToPipeline("videomixer name="
                         + compositorName
                         + " sink_1::alpha=0.4 " 
                         + "sink_0::alpha=1.0 background=3 ! "
                         + "videoconvert ! ");

  // Display processed video
  GstVideoPostProcess postProcess;
  pipeline.enablePerfDisplay(options.freq, options.time, 15, options.textColor);
  postProcess.display(pipeline, true);

  // Parse pipeline to GStreamer pipeline
  pipeline.parse(argc, argv, options.graphPath);

  // Run GStreamer pipeline
  pipeline.run();

  return 0;
}