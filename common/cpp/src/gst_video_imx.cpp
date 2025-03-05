/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unordered_map>
#include "gst_video_imx.hpp"


using FormatMap = std::unordered_map<std::string, bool>;


/** 
 * @brief Array of SoC features.
 */
std::unordered_map<std::string, FormatMap> isFormatSupported = {
    {"i.MX 8M Plus", {
        {"RGB16", true}, {"RGBx", true}, {"RGBA", true}, {"BGRA", true}, {"BGRx", true},
        {"BGR16", true}, {"ARGB", true}, {"ABGR", true}, {"xRGB", true}, {"xBGR", true},
        {"UYVY", false}, {"YUY2", false}, {"NV12", false}, {"GRAY8", false}, {"BGR", false}
    }},
    {"i.MX 93", {
        {"RGB16", true}, {"RGBx", false}, {"RGBA", false}, {"BGRA", true}, {"BGRx", true},
        {"BGR16", false}, {"ARGB", false}, {"ABGR", false}, {"xRGB", false}, {"xBGR", false},
        {"UYVY", true}, {"YUY2", false}, {"NV12", false}, {"GRAY8", true}, {"BGR", true}
    }},
    {"i.MX 95", {
        {"RGB16", true}, {"RGBx", true}, {"RGBA", true}, {"BGRA", true}, {"BGRx", true},
        {"BGR16", true}, {"ARGB", true}, {"ABGR", true}, {"xRGB", true}, {"xBGR", true},
        {"UYVY", true}, {"YUY2", true}, {"NV12", true}, {"GRAY8", true}, {"BGR", false}
    }}
};


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
  std::string name;
  int dimLimit = 16;
  bool validFormat;

  if (format.length() != 0) {
    cmdFormat = ",format=" + format;
    validFormat = isFormatSupported[imx.socName()][format];
  }

  if (this->imx.hasGPU2d()
      && (width > dimLimit || width == -1)
      && (height > dimLimit || height == -1)
      && (useCPU == false)
      && validFormat) {
    name = (flip == true) ? "name=scale_csc_flip_g2d_" : "name=scale_csc_g2d_";
    name += std::to_string(pipeline.elemNameCount);
    name += " ";
    cmd = "imxvideoconvert_g2d ";
    cmd += name;
    cmd += (flip == true) ? "rotation=4 ! " : "! ";
  } else if (this->imx.hasPxP()
           && (width > dimLimit || width == -1)
           && (height > dimLimit|| height == -1)
           && (useCPU == false)
           && validFormat) {
    name = (flip == true) ? "name=scale_csc_flip_pxp_" : "name=scale_csc_pxp_";
    name += std::to_string(pipeline.elemNameCount);
    name += " ";
    cmd = "imxvideoconvert_pxp ";
    cmd += name;
    cmd += (flip == true) ? "rotation=4 ! " : "! ";
  } else {
    /* no acceleration */
    name = "name=scale_cpu_" + std::to_string(pipeline.elemNameCount);
    cmd = "videoscale ";
    cmd += name;
    cmd += " ! ";

    name = "name=csc_cpu_" + std::to_string(pipeline.elemNameCount);
    cmd += "videoconvert ";
    cmd += name;
    cmd += " ";
    cmd += (flip == true) ? "! videoflip video-direction=4 ! " : "! ";
  }
  pipeline.elemNameCount += 1;

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
  std::string name;
  if (this->imx.hasGPU2d()) {
    /**
     * imxvideoconvert_g2d does not support RGB sink
     * and use CPU to convert RGBA to RGB
     */ 
    videoTransform(pipeline, "RGBA", width, height, false);
    name = "name=rgb_convert_cpu_" + std::to_string(pipeline.elemNameCount);
    pipeline.elemNameCount += 1;
    cmd = "videoconvert ";
    cmd += name;
    cmd += " ! video/x-raw,format=RGB ! ";
    pipeline.addToPipeline(cmd);
  } else if (this->imx.hasPxP()) {
    /** 
     * imxvideoconvert_pxp does not support RGB sink
     * and use CPU to convert BGR to RGB
     */
    videoTransform(pipeline, "BGR", width, height, false);
    name = "name=rgb_convert_cpu_" + std::to_string(pipeline.elemNameCount);
    pipeline.elemNameCount += 1;
    cmd = "videoconvert ";
    cmd += name;
    cmd += " ! video/x-raw,format=RGB ! ";
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
 * @param newWidth: output video width after crop.
 * @param newHeight: output video height after crop.
 */
void GstVideoImx::videocrop(GstPipelineImx &pipeline,
                            const std::string &gstName, 
                            const int &newWidth,
                            const int &newHeight)
{
  int width = pipeline.getDisplayWidth();
  int height = pipeline.getDisplayHeight();

  std::string cmd;
  cmd = "videocrop name=" + gstName;

  if ((newWidth > 0) || (newHeight > 0)) {
    int left = (width - newWidth)/2;
    int right = left;
    if ((left * 2 + newWidth) != width)
      right += 1;
    int top = (height - newHeight)/2;
    int bottom = top;
    if ((top * 2 + newHeight) != height)
      bottom += 1;
    cmd += " top=" + std::to_string(top) + " ";
    cmd += "bottom=" + std::to_string(bottom) + " ";
    cmd += "left=" + std::to_string(left) + " ";
    cmd += "right=" + std::to_string(right) + " ! ";
    if (imx.hasGPU2d())
      cmd += "imxvideoconvert_g2d videocrop-meta-enable=true ! ";
    cmd += "video/x-raw,width=" + std::to_string(newWidth) +
            ",height=" + std::to_string(newHeight);
  }
  cmd += " ! ";
  pipeline.addToPipeline(cmd);
}