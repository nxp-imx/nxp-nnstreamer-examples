/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_GST_SOURCE_IMX_H_
#define CPP_GST_SOURCE_IMX_H_

#include <string>
#include <filesystem>
#include <gst/pbutils/pbutils.h>

#include "gst_video_imx.hpp"
#include "gst_pipeline_imx.hpp"
#include "imx_devices.hpp"

typedef struct {
  std::filesystem::path cameraDevice;
  std::string gstName;
  int width;
  int height;
  bool horizontalFlip;
  std::string format;
  int framerate;
} CameraOptions;


/**
 * @brief Parent class for the various input source.
 */
class GstSourceImx {
  protected:
    int width;
    int height;
    std::string format;

  public:
    GstSourceImx(const int &width, const int &height, const std::string &format)
        : format(format), width(width), height(height) {}

    int getWidth() const { return width; }

    int getHeight() const { return height; }
};


/**
 * @brief Create pipeline segments for camera.
 */
class GstCameraImx : public GstSourceImx {
  private:
    std::string gstName;
    bool flip;
    GstVideoImx videoscale{};
    std::filesystem::path device;
    int framerate;

  public:
    GstCameraImx(CameraOptions &options);

    void addCameraToPipeline(GstPipelineImx &pipeline);
};


/**
 * @brief Create pipeline segments for a video.
 */
class GstVideoFileImx : public GstSourceImx {
  private:
    std::filesystem::path videoPath;
    int videoWidth;
    int videoHeight;
    bool newDim;
    std::string cmdDecoder;
    imx::Imx imx{};
    GstVideoImx videoscale{};
    bool loop;

  public:
    GstVideoFileImx(const std::filesystem::path &path,
                    const bool &loop=false,
                    const int &width=-1,
                    const int &height=-1);

    void addVideoToPipeline(GstPipelineImx &pipeline);
};


/**
 * @brief Create pipeline segments for a slideshow.
 */
class GstSlideshowImx : public GstSourceImx {
  private:
    std::filesystem::path slideshowPath;
    GstVideoImx videoscale{};

  public:
    GstSlideshowImx(const std::filesystem::path &path,
                    const int &width=-1,
                    const int &height=-1);

    void addSlideshowToPipeline(GstPipelineImx &pipeline);
};


/**
 * @brief Create pipeline segments for appsrc.
 */
class GstAppSrcImx : public GstSourceImx {
  private:
    std::string gstName;
    bool isLive;
    bool emitSignal;
    int maxBuffers;
    GstQueueLeaky leakType;
    int formatType;
    int framerate;
    GstVideoImx videoscale{};

  public:
    GstAppSrcImx(const std::string &gstName,
                 const bool &isLive,
                 const bool &emitSignal,
                 const int &maxBuffers,
                 const GstQueueLeaky &leakType,
                 const int &formatType,
                 const int &width,
                 const int &height,
                 const std::string &format="",
                 const int &framerate=30);
    
    void addAppSrcToPipeline(GstPipelineImx &pipeline);
};
#endif