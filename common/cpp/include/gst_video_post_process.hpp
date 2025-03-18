/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */

#ifndef CPP_GST_VIDEO_POST_PROCESS_H_
#define CPP_GST_VIDEO_POST_PROCESS_H_

#include <map>
#include <string>
#include <filesystem>
#include <vector>

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


enum class displayPosition {
  left,
  right,
  center
};


typedef struct {
  displayPosition position;
  int order;
  bool keepRatio;
  bool transparency;
} compositorInputParams;


/**
 * @brief Create pipeline segments for video post processing.
 */
class GstVideoPostProcess {
  private:
    static inline bool cairoNeeded = true;

  public:
    GstVideoPostProcess() = default;

    void display(GstPipelineImx &pipeline,
                 PerformanceType &perfType,
                 const std::string &color="");

    void addTextOverlay(GstPipelineImx &pipeline, 
                        const TextOverlayOptions &options);

    void addCairoOverlay(GstPipelineImx &pipeline, const std::string &gstName);
   
    void saveToVideo(GstPipelineImx &pipeline,
                     const std::string &format,
                     const std::filesystem::path &path);
    
    void addAppSink(GstPipelineImx &pipeline,
                    const AppSinkOptions &options);
};


/**
 * @brief Create pipeline segments for video compositor.
 */
class GstVideoCompositorImx {
  private:
    imx::Imx imx{};
    std::string gstName;
    std::vector<compositorInputParams> compositorInputs;
    static inline int sinkNumber = 0;

  public:
    GstVideoCompositorImx(const std::string &gstName)
        : gstName(gstName) {}

    void addToCompositor(GstPipelineImx &pipeline,
                         const compositorInputParams &inputParams);
    
    void addCompositorToPipeline(GstPipelineImx &pipeline,
                                 const int &latency = 0);
};
#endif