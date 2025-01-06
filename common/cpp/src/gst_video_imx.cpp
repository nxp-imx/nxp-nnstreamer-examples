/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gst_video_imx.hpp"

/**
 * @brief Create pipeline segment for accelerated video formatting and csc.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param format: GStreamer video format.
 * @param width: output video width after rescale.
 * @param  height: output video height after rescale.
 * @param flip: horizontal flip, deactivated by default.
 * @param aspectRatio: add pixel aspect ratio of 1/1, deactivated by default.
 * @param useCPU: use CPU instead of acceleration (false by default).
 */
void GstVideoImx::videoTransform(GstPipelineImx &pipeline,
                                 const std::string &format, 
                                 const int &width,
                                 const int &height,
                                 const bool &flip,
                                 const bool &aspectRatio,
                                 const bool &useCPU)
{
  /**
   * imxvideoconvert_g2d and imxvideoconvert_pxp
   * do not support width and height lower than 16
   */
  std::string cmd;
  std::string cmdFormat;
  int dimLimit = 16;
  bool validFormatG2D = true;
  bool validFormatPXP = true;

  if (format.length() != 0) {
    cmdFormat = ",format=" + format;
    /* accelerators only accept the following format*/
    if ((format != "RGB16")
        && (format != "RGBx")
        && (format != "RGBA")
        && (format != "BGRA")
        && (format != "BGRx")
        && (format != "BGR16")
        && (format != "ARGB")
        && (format != "ABGR")
        && (format != "xRGB")
        && (format != "xBGR")) {
      validFormatG2D = false;
    }
    if ((format != "BGRx")
        && (format != "BGRA")
        && (format != "BGR")
        && (format != "RGB16")
        && (format != "GRAY8")
        && (format != "UYVY")) {
      validFormatPXP = false;
    }
  }

  if (this->imx.hasGPU2d()
      && (width > dimLimit || width == -1)
      && (height > dimLimit || height == -1)
      && (useCPU == false)
      && (validFormatG2D == true)) {
    cmd = "imxvideoconvert_g2d ";
    cmd += (flip == true) ? "rotation=4 ! " : "! ";
  } else if (this->imx.hasPxP()
           && (width > dimLimit || width == -1)
           && (height > dimLimit|| height == -1)
           && (useCPU == false)
           && (validFormatPXP == true)) {
    cmd = "imxvideoconvert_pxp ";
    cmd += (flip == true) ? "rotation=4 ! " : "! ";
  } else {
    /* no acceleration */
    cmd = "videoscale ! videoconvert ";
    cmd += (flip == true) ? "! videoflip video-direction=4 ! " : "! ";
  }

  if (width > 0 && height > 0) {
    cmd += "video/x-raw,width=" + std::to_string(width) + ",";
    cmd += "height=" + std::to_string(height) + cmdFormat;
    cmd += (aspectRatio == true) ? ",pixel-aspect-ratio=1/1 ! " : " ! ";
  } else {
    if (format.length() != 0)
      cmd += "video/x-raw" + cmdFormat + " ! ";
  }

  pipeline.addToPipeline(cmd);
}


/**
 * @brief Create pipeline segment for accelerated video scaling and
 *        conversion to RGB format.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param width: output video width after rescale.
 * @param height: output video height after rescale.
 */
void GstVideoImx::videoscaleToRGB(GstPipelineImx &pipeline,
                                  const int &width,
                                  const int &height)
{
  std::string cmd;
  if (this->imx.hasGPU2d()) {
    /**
     * imxvideoconvert_g2d does not support RGB sink
     * and use CPU to convert RGBA to RGB
     */ 
    videoTransform(pipeline, "RGBA", width, height, false);
    cmd = "videoconvert ! video/x-raw,format=RGB ! ";
    pipeline.addToPipeline(cmd);
  } else if (this->imx.hasPxP()) {
    /** 
     * imxvideoconvert_pxp does not support RGB sink
     * and use CPU to convert BGR to RGB
     */
    videoTransform(pipeline, "BGR", width, height, false);
    cmd = "videoconvert ! video/x-raw,format=RGB ! ";
    pipeline.addToPipeline(cmd);
  } else {
    /* no acceleration */
    videoTransform(pipeline, "RGB", width, height, false);
  }
}


/**
 * @brief Create pipeline segment for accelerated video cropping.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param gstName: GStreamer videocrop element name.
 * @param width: output video width after rescale.
 * @param height: output video height after rescale.
 * @param top: top pixels to be cropped, default is 0.
 * @param bottom: bottom pixels to be cropped, default is 0.
 * @param left: left pixels to be cropped, default is 0.
 * @param right: right pixels to be cropped, default is 0.
 */
void GstVideoImx::videocrop(GstPipelineImx &pipeline,
                            const std::string &gstName, 
                            const int &width,
                            const int &height,
                            const int &top,
                            const int &bottom,
                            const int &left,
                            const int &right)
{
  std::string cmd;
  cmd = "videocrop name=" + gstName + " ";
  if(top != 0)
    cmd += "top=" + std::to_string(top) + " ";
  if(bottom != 0)
    cmd += "bottom=" + std::to_string(bottom) + " ";
  if(left != 0)
    cmd += "left=" + std::to_string(left) + " ";
  if(right != 0)
    cmd += "right=" + std::to_string(right) + " ";
  cmd += "! ";
  if (width > 0 && height > 0) {
    cmd += "video/x-raw,width=" + std::to_string(width) +
           ",height=" + std::to_string(height) + " ! ";
  }
  pipeline.addToPipeline(cmd);
}


/**
 * @brief Create pipeline segment for accelerated video mixing.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param gstName: GStreamer compositor element name.
 * @param latency: time for a capture to reach the sink, default 0.
 * @param position: displayPosition structure, default is displayPosition::centered.
 */
void GstVideoImx::videoCompositor(GstPipelineImx &pipeline,
                                  const std::string &gstName,
                                  const int &latency,
                                  const displayPosition &position)
 {
  std::string cmd;
  std::string firstStream = "sink_0";
  std::string secondStream = "sink_1";
  
  if(this->imx.hasGPU2d()) {
    cmd = "imxcompositor_g2d name=" + gstName + " ";
    cmd += firstStream + "::zorder=2 ";
    cmd += secondStream + "::zorder=1 ";
  } else if(this->imx.hasPxP()) {
    /** 
     * imxcompositor_pxp does not support RGBA sink
     * and use CPU to convert RGBA to RGB
     */ 
    cmd = "imxcompositor_pxp name=" + gstName + " ";
    cmd += firstStream + "::zorder=2 ";
    cmd += secondStream + "::zorder=1 ";
    if (position == displayPosition::mixed)
      cmd += firstStream + "::alpha=0.3 ";
  } else {
    /*  no acceleration */
    cmd = "compositor name=" + gstName + " ";
  }

  if(latency != 0) {
    cmd += "latency=" + std::to_string(latency);
    cmd += " min-upstream-latency=" + std::to_string(latency) + " ";
  }

  if (position == displayPosition::split) {
    cmd += "sink_0::xpos=0 sink_0::ypos=0 sink_0::width=960 sink_0::height=720 ";
    cmd += "sink_1::xpos=960 sink_1::ypos=0 sink_1::width=960 sink_1::height=720 ";
    cmd += "sink_0::keep-ratio=true sink_1::keep-ratio=true ";
  }
  cmd += "! ";
  pipeline.addToPipeline(cmd);
}