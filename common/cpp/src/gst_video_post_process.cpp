/**
 * Copyright 2024 NXP
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

  std::string cmd = "textoverlay name=" + options.name + " font-desc=\"" + options.fontName;                             
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
 * @param name: name for GStreamer textoverlay element.
 */ 
void GstVideoPostProcess::addCairoOverlay(GstPipelineImx &pipeline,
                                          const std::string &name)
{
  imx::Imx imx{};
  if (imx.hasGPU2d()){
    std::string cmd = "imxvideoconvert_g2d ! cairooverlay name=" + name + " ! ";
    pipeline.addToPipeline(cmd);
  }
  else if (imx.hasPxP()){
    std::string cmd = "imxvideoconvert_pxp ! cairooverlay name=" + name + " ! ";
    pipeline.addToPipeline(cmd);
  }
  else{
    std::string cmd = "videoconvert ! cairooverlay name=" + name + " ! ";
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
  if (imx.isIMX9()) {
    log_error("video file can't be encoded with %s\n", imx.socName().c_str());
    exit(-1);
  } else {
    std::string cmd;
    if (format == "mkv") {
      cmd = "vpuenc_h264 ! h264parse ! matroskamux ! filesink location=";
      cmd += path.string() + " ";
      pipeline.addToPipeline(cmd);
    }

    if (format == "mp4") {
      cmd = "vpuenc_h264 ! h264parse ! qtmux ! filesink location=";
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
                                     AppSinkOptions &options)
{
  std::string cmd;
  cmd = "appsink";

  if (options.name.size() != 0)
    cmd += " name=" + options.name;
  if (options.sync == false)
     cmd += " sync=false";
  cmd += " max-buffers=" + std::to_string(options.maxBuffers);

  if (options.drop == true)
     cmd += " drop=true";
  if (options.emitSignals == true)
     cmd += " emit-signals=true";

  pipeline.addToPipeline(cmd + " ");
}