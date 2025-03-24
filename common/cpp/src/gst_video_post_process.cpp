/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gst_video_post_process.hpp"

/** 
 * @brief Dictionary of color in big-endian ARGB.
 */ 
std::map<std::string, int> DictionaryColorARGB = {
  {"red",   0xFFFF0000},
  {"green", 0xFF00FF00},
  {"blue",  0xFF0000FF},
  {"black", 0xFF000000},
};


/**
 * @brief Display GStreamer pipeline output.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param sync: specifiy if we synchronized display with input buffer.
 */
void GstVideoPostProcess::display(GstPipelineImx &pipeline,
                                  const bool &sync)
{
  std::string cmdSync = (sync == false) ? "sync=false " : "";

  std::string cmd;
  if ((pipeline.isPerfAvailable().freq == true) || (pipeline.isPerfAvailable().temp == true)) {
    if (cairoNeeded == true) {
      cmd = "cairooverlay name=perf ! ";
      cairoNeeded = false;
    }
    cmd += "fpsdisplaysink name=img_tensor text-overlay=false video-sink=waylandsink " + cmdSync;
  } else {
    cmd = "waylandsink " + cmdSync;
  }

  pipeline.addToPipeline(cmd);
}


/**
 * @brief Add to pipeline an element to display text.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param options: TextOverlayOptions structure, setup textoverlay options.
 */
void GstVideoPostProcess::addTextOverlay(GstPipelineImx &pipeline, 
                                         const TextOverlayOptions &options)
{
  imx::Imx imx{};
  std::string colorOption;
  if (options.color.length() != 0)
    colorOption = (" color=" + std::to_string(DictionaryColorARGB[options.color]));

  std::string textOption;
  if (options.text.length() != 0)
    textOption = (" text=" + options.text);

  std::string vAlignOption;
  if (options.vAlignment.length() != 0)
    vAlignOption = (" valignment=" + options.vAlignment);

  std::string hAlignOption;
  if (options.hAlignment.length() != 0)
    hAlignOption = (" halignment=" + options.hAlignment);

  std::string cmd = "textoverlay name=" + options.gstName + " font-desc=\"" + options.fontName;
  cmd += ", " + std::to_string(options.fontSize) + "\"" + colorOption + textOption;
  if(imx.hasGPU2d())
    cmd += vAlignOption + hAlignOption + " ! imxvideoconvert_g2d ! ";
  else if(imx.hasPxP())
    cmd += vAlignOption + hAlignOption + " ! imxvideoconvert_pxp ! ";
  else
    cmd += vAlignOption + hAlignOption + " ! videoconvert ! ";

  pipeline.addToPipeline(cmd);
}


/**
 * @brief Add to pipeline cairooverlay for custom drawing.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param gstName: name for GStreamer textoverlay element.
 */ 
void GstVideoPostProcess::addCairoOverlay(GstPipelineImx &pipeline,
                                          const std::string &gstName)
{
  imx::Imx imx{};
  if (imx.hasGPU2d()){
    std::string cmd = "imxvideoconvert_g2d ! cairooverlay name=" + gstName + " ! ";
    pipeline.addToPipeline(cmd);
  }
  else if (imx.hasPxP()){
    std::string cmd = "imxvideoconvert_pxp ! cairooverlay name=" + gstName + " ! ";
    pipeline.addToPipeline(cmd);
  }
  else{
    std::string cmd = "videoconvert ! cairooverlay name=" + gstName + " ! ";
    pipeline.addToPipeline(cmd);
  }
  
}


/**
 * @brief Add to pipeline an element to save pipeline to video.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param format: saved video format. Only MKV and MP4 format supported.
 * @param path: path to save video.
 */       
void GstVideoPostProcess::saveToVideo(GstPipelineImx &pipeline,
                                      const std::string &format,
                                      const std::filesystem::path &path)
{
  /**
   * webm not supported since no vp9 encoder in GStreamer version used
   */
  pipeline.setSave(true);
  imx::Imx imx{};
  if (imx.socId() == imx::IMX93) {
    log_error("video file can't be encoded with %s\n", imx.socName().c_str());
    exit(-1);
  } else {
    std::string cmd;
    cmd = "v4l2h265enc ! h265parse ! ";

    if (format == "mkv") {
      cmd += "matroskamux ! filesink location=";
      cmd += path.string() + " ";
      pipeline.addToPipeline(cmd);
    }

    if (format == "mp4") {
      cmd = "qtmux ! filesink location=";
      cmd += path.string() + " ";
      pipeline.addToPipeline(cmd);
    }
  }   
}


/**
 * @brief Add appsink element which allows the application to get access
 *        to the raw buffer from the GStreamer pipeline.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param options: AppSinkOptions structure, setup appsink options.
 */  
void GstVideoPostProcess::addAppSink(GstPipelineImx &pipeline,
                                     const AppSinkOptions &options)
{
  std::string cmd;
  cmd = "appsink";

  if (options.gstName.size() != 0)
    cmd += " name=" + options.gstName;
  if (options.sync == false)
     cmd += " sync=false";
  cmd += " max-buffers=" + std::to_string(options.maxBuffers);

  if (options.drop == true)
     cmd += " drop=true";
  if (options.emitSignals == true)
     cmd += " emit-signals=true";

  pipeline.addToPipeline(cmd + " ");
}


/**
 * @brief Link video to compositor.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param inputParams: structure of parameters for an input video.
 */
void GstVideoCompositorImx::addToCompositor(GstPipelineImx &pipeline,
                                            const compositorInputParams &inputParams)
{
  pipeline.addToPipeline(this->gstName + ".sink_" + std::to_string(sinkNumber) + " ");
  this->compositorInputs.push_back(inputParams);
  sinkNumber += 1;
}

/**
 * @brief Create pipeline segment for accelerated video mixing.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param latency: time for a capture to reach the sink, default 0.
 */
void GstVideoCompositorImx::addCompositorToPipeline(GstPipelineImx &pipeline,
                                                    const int &latency)
 {
  std::string cmd;
  std::string customParams;

  for (int i=0; i < this->compositorInputs.size(); i++) {
    compositorInputParams inputParams = compositorInputs.at(i);
    std::string stream = "sink_" + std::to_string(i);
    customParams += stream + "::zorder=" + std::to_string(inputParams.order) + " ";

    if (this->imx.hasPxP() && (inputParams.transparency == true))
      customParams += stream + "::alpha=0.3 ";
  
    switch (inputParams.position)
    {
      case displayPosition::left:
        customParams += stream + "::xpos=0 ";
        customParams += stream + "::ypos=0 ";
        customParams += stream + "::width=960 ";
        customParams += stream + "::height=720 ";
        if (inputParams.keepRatio == true)
          customParams += stream + "::keep-ratio=true ";
        break;

      case displayPosition::right:
        customParams += stream + "::xpos=960 ";
        customParams += stream + "::ypos=0 ";
        customParams += stream + "::width=960 ";
        customParams += stream + "::height=720 ";
        if (inputParams.keepRatio == true)
          customParams += stream + "::keep-ratio=true ";
        break;

      case displayPosition::center:
        break;

      default:
        break;
    }
  }

  if(this->imx.hasGPU2d()) {
    cmd = "imxcompositor_g2d name=" + this->gstName + " ";
    cmd += customParams;
  } else if(this->imx.hasPxP()) {
    /** 
     * imxcompositor_pxp does not support RGBA sink
     * and use CPU to convert RGBA to RGB
     */ 
    cmd = "imxcompositor_pxp name=" + this->gstName + " ";
    cmd += customParams;
  } else {
    /*  no acceleration */
    cmd = "compositor name=" + gstName + " ";
    cmd += customParams;
  }

  if(latency != 0) {
    cmd += "latency=" + std::to_string(latency);
    cmd += " min-upstream-latency=" + std::to_string(latency);
    cmd += " ";
  }

  cmd += "! ";
  pipeline.addToPipeline(cmd);
}