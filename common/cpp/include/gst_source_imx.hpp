/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_GST_SOURCE_IMX_H_
#define CPP_GST_SOURCE_IMX_H_

#include <string>
#include <filesystem>

#include "gst_video_imx.hpp"
#include "gst_pipeline_imx.hpp"
#include "imx_devices.hpp"

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
    std::string name;
    bool flip;
    GstVideoImx videoscale{};
    std::filesystem::path device;
    int framerate;

  public:
    GstCameraImx(const std::filesystem::path &cameraDevice,
                 const std::string &name,
                 const int &width,
                 const int &height,
                 const bool &flip,
                 const std::string &format="",
                 const int &framerate=30);

    void addCameraToPipeline(GstPipelineImx &pipeline);
};


/**
 * @brief Create pipeline segments for a video.
 */
class GstVideoFileImx : public GstSourceImx {
  private:
    std::filesystem::path videoPath;
    std::string cmdDecoder;
    imx::Imx imx{};
    GstVideoImx videoscale{};

  public:
    GstVideoFileImx(const std::filesystem::path &path,
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
    std::string name;
    bool isLive;
    bool emitSignal;
    int maxBuffers;
    GstQueueLeaky leakType;
    int formatType;
    int framerate;
    GstVideoImx videoscale{};

  public:
    GstAppSrcImx(const std::string &name,
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