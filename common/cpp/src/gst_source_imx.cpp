/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "gst_source_imx.hpp"

/**
 * @brief Parameterized constructor.
 * 
 * @param options: structure of camera parameters.
 */
GstCameraImx::GstCameraImx(CameraOptions &options)
    : GstSourceImx(options.width, options.height, options.format),
      flip(options.horizontalFlip),
      gstName(options.gstName),
      framerate(options.framerate)
{
  if (options.cameraDevice.string().length() != 0) {
    device = options.cameraDevice;
  } else {
    imx::Imx imx{};
    switch (imx.socId())
    {
      case imx::IMX8MP:
        device = "/dev/video3";
        break;

      case imx::IMX93:
        device = "/dev/video0";
        break;

      case imx::IMX95:
        device = "/dev/video13";
        break;

      default:
        log_error("Select a camera device using -c argument option\n");
        break;
    }
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
  cmd += "v4l2src name=" + gstName + " device=" + device.string();
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
  if ((imx.socId() == imx::IMX93) || (imx.socId() == imx::IMX95)) {
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
 * @param gstName: name for GStreamer appsrc element.
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
GstAppSrcImx::GstAppSrcImx(const std::string &gstName,
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
      gstName(gstName),
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

  if (gstName.size() != 0)
    cmd += " name=" + gstName;
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