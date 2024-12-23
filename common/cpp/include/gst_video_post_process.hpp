/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */

#ifndef CPP_GST_VIDEO_POST_PROCESS_H_
#define CPP_GST_VIDEO_POST_PROCESS_H_

#include <map>
#include <string>
#include <filesystem>

#include "gst_pipeline_imx.hpp"


typedef struct {
  std::string gstName;
  std::string fontName;
  int fontSize;
  std::string color;
  std::string vAlignment;
  std::string hAlignment;
  std::string text;
} TextOverlayOptions;


typedef struct {
  std::string gstName;
  bool sync;
  int maxBuffers;
  bool drop;
  bool emitSignals;
} AppSinkOptions;


/**
 * @brief Create pipeline segments for video post processing.
 */
class GstVideoPostProcess {
  private:
    static inline bool cairoNeeded = true;

  public:
    GstVideoPostProcess() = default;

    void display(GstPipelineImx &pipeline,
                 const bool &sync=false);

    void addTextOverlay(GstPipelineImx &pipeline, 
                        const TextOverlayOptions &options);

    void addCairoOverlay(GstPipelineImx &pipeline, const std::string &gstName);
   
    void saveToVideo(GstPipelineImx &pipeline,
                     const std::string &format,
                     const std::filesystem::path &path);
    
    void addAppSink(GstPipelineImx &pipeline,
                    AppSinkOptions &options);
};
#endif