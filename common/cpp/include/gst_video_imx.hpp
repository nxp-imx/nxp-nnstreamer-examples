/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_GST_VIDEO_IMX_H_
#define CPP_GST_VIDEO_IMX_H_

#include "imx_devices.hpp"
#include "gst_pipeline_imx.hpp"

enum class displayPosition {
  mixed,
  split
};


/**
 * @brief Create pipeline segments for the various accelerators.
 */
class GstVideoImx {
  private:
    imx::Imx imx{};

  public:
    GstVideoImx() = default;

    void videoTransform(GstPipelineImx &pipeline,
                        const std::string &format,
                        const int &width,
                        const int &height,
                        const bool &flip,
                        const bool &aspectRatio=false,
                        const bool &useCPU=false);

    void videoscaleToRGB(GstPipelineImx &pipeline,
                         const int &width,
                         const int &height);

    void videocrop(GstPipelineImx &pipeline,
                   const std::string &name,
                   const int &width,
                   const int &height,
                   const int &top=0,
                   const int &bottom=0,
                   const int &left=0,
                   const int &right=0);

    void videoCompositor(GstPipelineImx &pipeline,
                         const std::string &name,
                         const int &latency = 0,
                         const displayPosition &position=displayPosition::mixed);
};
#endif