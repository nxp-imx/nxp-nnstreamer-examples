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
  pipeline.setDisplayResolution(this->width, this->height);

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
 * @param loop: loop video.
 * @param width: video width.
 * @param height: video height.
 */
GstVideoFileImx::GstVideoFileImx(const std::filesystem::path &path,
                                 const bool &loop,
                                 const int &width,
                                 const int &height)
    : GstSourceImx(width, height, ""), videoPath(path), loop(loop)
{
  if (imx.isIMX93()) {
    log_error("Video file decoding is not available on i.MX 93 \n");
    exit(-1);
  }

  // Determine format
  std::string extension = path.extension().string();
  if (extension == ".mkv" || extension == ".webm") {
    cmdDecoder = "matroskademux ! ";
  } else if (extension == ".mp4"
            || extension == ".mov"
            || extension == ".m4a"
            || extension == ".3gp"
            || extension == ".3g2"
            || extension == ".mj2") {
    cmdDecoder = "qtdemux ! ";
  } else {
    log_error("Unsupported format. Only matroska/webm/mp4/mov are supported.\n");
    exit(-1);
  }

  GError *err = nullptr;
  GstDiscoverer *discoverer = gst_discoverer_new(5 * GST_SECOND, &err);
  if (!discoverer) {
    log_error("Failed to create GstDiscoverer: %s\n", err->message);
    g_clear_error(&err);
    exit(-1);
  }

  std::string uri = "file://" + path.string();
  GstDiscovererInfo *info = gst_discoverer_discover_uri(discoverer, uri.c_str(), &err);
  if (!info) {
    log_error("Failed to discover URI: %s\n", err->message);
    g_clear_error(&err);
    g_object_unref(discoverer);
    exit(-1);
  }

  GList *video_streams = gst_discoverer_info_get_video_streams(info);
  if (!video_streams) {
    log_error("No video stream found in file: %s\n", path.c_str());
    g_list_free(video_streams);
    g_object_unref(info);
    g_object_unref(discoverer);
    exit(-1);
  }

  GstDiscovererStreamInfo *stream_info = static_cast<GstDiscovererStreamInfo *>(video_streams->data);
  GstCaps *caps = gst_discoverer_stream_info_get_caps(stream_info);
  if (!caps) {
    log_error("Failed to get caps from video stream\n");
    g_list_free(video_streams);
    g_object_unref(info);
    g_object_unref(discoverer);
    exit(-1);
  }

  const GstStructure *structure = gst_caps_get_structure(caps, 0);
  const gchar *media_type = gst_structure_get_name(structure);

  // Determine decoder
  if (g_strcmp0(media_type, "video/x-vp9") == 0) {
    cmdDecoder += "vp9parse ! ";
    if (imx.isIMX8())
      cmdDecoder += "v4l2vp9dec ! ";
    else if (imx.isIMX95()) // v4l2 vp9 decoder not available on IMX95
      cmdDecoder += "avdec_vp9 ! ";
  } else if (g_strcmp0(media_type, "video/x-h264") == 0) {
    cmdDecoder += "h264parse ! v4l2h264dec ! ";
  } else if (g_strcmp0(media_type, "video/x-h265") == 0) {
    cmdDecoder += "h265parse ! v4l2h265dec ! ";
  } else {
    log_error("Unsupported codec. Only VP9, H265, and H264 are supported.\n");
    gst_caps_unref(caps);
    g_list_free(video_streams);
    g_object_unref(info);
    g_object_unref(discoverer);
    exit(-1);
  }

  if (!gst_structure_get_int(structure, "width", &this->videoWidth)
      || !gst_structure_get_int(structure, "height", &this->videoHeight)) {
    log_error("Failed to extract video resolution from caps\n");
    gst_caps_unref(caps);
    g_object_unref(info);
    g_object_unref(discoverer);
    exit(-1);
  }

  log_debug("Detected resolution: %dx%d\n", this->videoWidth, this->videoHeight);

  if ((width == -1) || (height == -1) || ((width == this->videoWidth) && (height == this->videoHeight))) {
    this->width = this->videoWidth;
    this->height = this->videoHeight;
    this->newDim = false;
  } else {
    this->newDim = true;
  }

  gst_caps_unref(caps);
  g_list_free(video_streams);
  g_object_unref(info);
  g_object_unref(discoverer);
}


/**
 * @brief Create pipeline segment for video.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 */
void GstVideoFileImx::addVideoToPipeline(GstPipelineImx &pipeline)
{
  pipeline.setDisplayResolution(this->width, this->height);

  std::string cmd;   
  cmd += "filesrc location=" + videoPath.string() + " ! " + this->cmdDecoder;
  cmd += "video/x-raw,width=" + std::to_string(this->videoWidth)
         + ",height=" + std::to_string(this->videoHeight) + " ! ";
  pipeline.addToPipeline(cmd);

  if (this->newDim == true)
    videoscale.videoTransform(pipeline, "", this->width, this->height, false, true);

    if (this->loop)
      pipeline.loopPipeline();
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
  pipeline.setDisplayResolution(this->width, this->height);

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