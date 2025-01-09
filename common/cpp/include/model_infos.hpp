/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_MODEL_INFOS_H_
#define CPP_MODEL_INFOS_H_

#include <filesystem>
#include <thread>

#include "imx_devices.hpp"
#include "gst_video_imx.hpp"
#include "gst_pipeline_imx.hpp"
#include "tensor_custom_data_generator.hpp"


/**
 * @brief Create pipeline segments for various models.
 */
class ModelInfos {
  protected:
    int modelWidth;
    int modelHeight;
    int modelChannel;
    std::filesystem::path modelPath;
    std::string backend;
    std::string framework;
    TensorCustomGenerator tensorCustomData;
    imx::Imx imx{};
    GstVideoImx videoscale{};
    TensorData tensorData;

  public:
    ModelInfos(const std::filesystem::path &path,
               const std::string &backend,
               const std::string &norm,
               const int &numThreads);

    int getModelWidth() const { return modelWidth; }

    int getModelHeight() const { return modelHeight; }

    int getModelChannel() const { return modelChannel; }

    bool isGrayscale() const  { return ((modelChannel == 1) ? true : false); }

    bool isRGB() const  { return ((modelChannel == 3) ? true : false); }

    void addInferenceToPipeline(GstPipelineImx &pipeline,
                                const std::string &gstName="",
                                const std::string &format="RGB");

    void setTensorFilterConfig(imx::Imx &imx, const int &numThreads);
};


/**
 * @brief Create pipeline segments for tensorflow lite model.
 */
class TFliteModelInfos : public ModelInfos {
  public:
    TFliteModelInfos(const std::filesystem::path &path,
                     const std::string &backend,
                     const std::string &norm,
                     const int &numThreads=std::thread::hardware_concurrency());
};
#endif