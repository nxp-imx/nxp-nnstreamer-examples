/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "gst_source_imx.hpp"

/**
 * @brief Parameterized constructor.
 * 
 * @param cameraDevice: camera device to use.
 * @param name: name for GStreamer camera element.
 * @param width: camera width.
 * @param height: camera height.
 * @param flip: camera horizontal flip.
 * @param format: GStreamer video format.
 * @param framerate: camera framerate.
 */
GstCameraImx::GstCameraImx(const std::filesystem::path &cameraDevice,
                           const std::string &name,
                           const int &width,
                           const int &height,
                           const bool &flip,
                           const std::string &format,
                           const int &framerate)
    : GstSourceImx(width, height, format),
      device(cameraDevice),
      flip(flip),
      name(name),
      framerate(framerate)
{
  if (cameraDevice.string().length() != 0) {
    device = cameraDevice;
  } else {
    imx::Imx imx{};
    device = (imx.socId() == imx::IMX8MP) ? "/dev/video3" : "/dev/video0";
  }

  if (width <= 320 and height <= 240) {
    this->width = 320;
    this->height = 240;
  } else {
    this->width = 640;
    this->height = 480;        
  }
}


/**
 * @brief Create pipeline segment for camera.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 */
void GstCameraImx::addCameraToPipeline(GstPipelineImx &pipeline)
{
  std::string cmd;
  cmd += "v4l2src name=" + name + " device=" + device.string();
  cmd += " num-buffers=-1 ! video/x-raw,width=";
  cmd += std::to_string(width) + ",height=" + std::to_string(height);
  cmd += ",framerate=" + std::to_string(framerate) + "/1 ! ";
  pipeline.addToPipeline(cmd);

  if ((format.length() != 0) or flip)
    videoscale.videoTransform(pipeline, format, -1, -1, flip, false, false);
}


/**
 * @brief  Parameterized constructor.
 * 
 * @param path: video path.
 * @param width: video width.
 * @param height: video height.
 */
GstVideoFileImx::GstVideoFileImx(const std::filesystem::path &path,
                                 const int &width,
                                 const int &height)
    : GstSourceImx(width, height, "")
{
  if (imx.isIMX9()) {
    log_error("video file can't be decoded with %s\n", imx.socName().c_str());
    exit(-1);
  } else {
    videoPath = path;
  }

  if (videoPath.extension() == ".mkv" or 
      videoPath.extension() == ".webm") {
    cmdDecoder = "matroskademux ! ";
  } else if (videoPath.extension() == ".mp4") {
    cmdDecoder = "qtdemux ! ";
  } else {
    log_error("Add a .mkv, .webm or .mp4 video format\n");
    exit(-1);
  }

  cmdDecoder += "vpudec ! ";
}


/**
 * @brief Create pipeline segment for video.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 */
void GstVideoFileImx::addVideoToPipeline(GstPipelineImx &pipeline)
{
  std::string cmd;   
  cmd += "filesrc location=" + videoPath.string() + " ! " + cmdDecoder;
  pipeline.addToPipeline(cmd);

  if ((width > 0) && (height > 0))
    videoscale.videoTransform(pipeline, "", width, height, false, true);
}


/**
 * @brief  Parameterized constructor.
 * 
 * @param path: slideshow path.
 * @param width: slideshow width.
 * @param height: slideshow height.
 */
GstSlideshowImx::GstSlideshowImx(const std::filesystem::path &path,
                                 const int &width,
                                 const int &height)
    : GstSourceImx(width, height, "") 
{
  slideshowPath = path;
}


/**
 * @brief Create pipeline segment for slideshow.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 */
void GstSlideshowImx::addSlideshowToPipeline(GstPipelineImx &pipeline)
{
  std::string cmd;
  cmd = "multifilesrc location=" + slideshowPath.string();
  cmd += " loop=true caps=image/jpeg,framerate=1/2 ! jpegdec ! ";
  pipeline.addToPipeline(cmd);

  if ((width > 0) && (height > 0))
    videoscale.videoTransform(pipeline, "", width, height, false, true);
}


/**
 * @brief Parameterized constructor.
 * 
 * @param name: name for GStreamer appsrc element.
 * @param isLive: whether to act as a live source.
 * @param emitSignal: emit need-data, enough-data and seek-data signals.
 * @param maxBuffers: the maximum number of buffers to queue internally.
 * @param leakType: whether to drop buffers once the internal queue is full.
 * @param formatType: the format of the segment events and seek.
 * @param width: appsrc width.
 * @param height: appsrc height.
 * @param format: GStreamer video format.
 * @param framerate: appsrc framerate.
 */
GstAppSrcImx::GstAppSrcImx(const std::string &name,
                           const bool &isLive,
                           const bool &emitSignal,
                           const int &maxBuffers,
                           const GstQueueLeaky &leakType,
                           const int &formatType,
                           const int &width,
                           const int &height,
                           const std::string &format,
                           const int &framerate)
    : GstSourceImx(width, height, format),
      name(name),
      isLive(isLive),
      emitSignal(emitSignal),
      maxBuffers(maxBuffers),
      leakType(leakType),
      formatType(formatType),
      framerate(framerate)
{
}


/**
 * @brief Create pipeline segment for appsrc element.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 */
void GstAppSrcImx::addAppSrcToPipeline(GstPipelineImx &pipeline)
{
  std::string cmd;
  std::string caps;
  cmd = "appsrc";

  if (name.size() != 0)
    cmd += " name=" + name;
  if (isLive == true)
     cmd += " is-live=true";

  caps += "video/x-raw,width=" + std::to_string(width);
  caps += ",height=" + std::to_string(height);
  caps += ",framerate=" + std::to_string(framerate) + "/1";

  if (format.size() != 0)
    caps += ",format=" + format;

  cmd += " caps=" + caps;
  cmd += " format=" + std::to_string(formatType);

  if (emitSignal == false)
    cmd += " emit-signals=false";

  cmd += " max-buffers=" + std::to_string(maxBuffers);

  if (leakType != GstQueueLeaky::no)
    cmd += " leaky-type=" + std::to_string(static_cast<int>(leakType));

  cmd += " ! " + caps + " ! ";
  pipeline.addToPipeline(cmd);
}