/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */

#include "tensor_custom_data_generator.hpp"

/**
 * @brief Map of available normalization.
 */
std::map<std::string, Normalization> normDictionary = {
  {"none", Normalization::none},
  {"centered", Normalization::centered},
  {"scaled", Normalization::scaled},
  {"centeredScaled", Normalization::centeredScaled},
  {"castInt32", Normalization::castInt32},
  {"castuInt8", Normalization::castuInt8},
};


/**
 * @brief Add tensor_filter option for CPU backend.
 *
 * @param numThreads: number of threads for XNNPACK (CPU backend).
 */
std::string TensorCustomGenerator::CPU(const int &numThreads)
{
  tensorData.tensorFilterCustom = "custom=Delegate:XNNPACK,";
  tensorData.tensorFilterCustom += "NumThreads:" + std::to_string(numThreads);
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add tensor_filter option for VSI GPU backend.
 */
std::string TensorCustomGenerator::vsiGPU()
{
  tensorData.tensorFilterCustom = "custom=Delegate:External,";
  tensorData.tensorFilterCustom += "ExtDelegateLib:libvx_delegate.so";
  setenv("USE_GPU_INFERENCE","1",1);
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add tensor_filter option for VSI NPU backend.
 */
std::string TensorCustomGenerator::vsiNPU()
{
  tensorData.tensorFilterCustom = "custom=Delegate:External,";
  tensorData.tensorFilterCustom += "ExtDelegateLib:libvx_delegate.so";
  setenv("USE_GPU_INFERENCE","0",1); 
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add tensor_filter option for Ethos-U NPU backend.
 */
std::string TensorCustomGenerator::ethosNPU()
{
  tensorData.tensorFilterCustom = "custom=Delegate:External,";
  tensorData.tensorFilterCustom += "ExtDelegateLib:libethosu_delegate.so";
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add tensor_filter option for Ethos-U NPU backend.
 */
std::string TensorCustomGenerator::neutronNPU()
{
  tensorData.tensorFilterCustom = "custom=Delegate:External,";
  tensorData.tensorFilterCustom += "ExtDelegateLib:libneutron_delegate.so";
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add tensor_filter option for GPU backend on i.MX95.
 */
std::string TensorCustomGenerator::GPU()
{
  tensorData.tensorFilterCustom = "custom=Delegate:GPU";
  return tensorData.tensorFilterCustom;
}


/**
 * @brief Add element for normalization to pipeline.
 */
std::string TensorCustomGenerator::setTensorTransformConfig(const std::string &norm, GstPipelineImx &pipeline)
{
  std::string tensorTransformCustom;
  std::string name;
  switch (selectFromDictionary(norm, normDictionary))
  {
    case Normalization::none:
      tensorTransformCustom = "";
      break;

    case Normalization::centered:
      name = "name=tensor_preprocess_centered_normalization_" + std::to_string(pipeline.elemNameCount) + " ";
      pipeline.elemNameCount += 1;
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += name;
      tensorTransformCustom += "option=typecast:int16,add:-128 ! ";
      tensorTransformCustom += "tensor_transform mode=typecast ";
      tensorTransformCustom += "option=int8 ! ";
      break;

    case Normalization::scaled:
      name = "name=tensor_preprocess_scaled_normalization_" + std::to_string(pipeline.elemNameCount) + " ";
      pipeline.elemNameCount += 1;
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += name;
      tensorTransformCustom += "option=typecast:float32,div:255 ! ";
      break;

    case Normalization::centeredScaled:
      name = "name=tensor_preprocess_centered_scaled_normalization_" + std::to_string(pipeline.elemNameCount) + " ";
      pipeline.elemNameCount += 1;
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += name;
      tensorTransformCustom += "option=typecast:float32,add:-127.5,div:127.5 ! ";
      break;

    case Normalization::castInt32:
      name = "name=int32_cast_" + std::to_string(pipeline.elemNameCount) + " ";
      pipeline.elemNameCount += 1;
      tensorTransformCustom = "tensor_transform mode=typecast ";
      tensorTransformCustom += name;
      tensorTransformCustom += "option=int32 ! ";
      break;

    case Normalization::castuInt8:
      name = "name=uint8_cast_" + std::to_string(pipeline.elemNameCount) + " ";
      pipeline.elemNameCount += 1;
      tensorTransformCustom = "tensor_transform mode=typecast ";
      tensorTransformCustom += name;
      tensorTransformCustom += "option=uint8 ! ";
      break;

    default:
      tensorTransformCustom = "";
      break;
  }
  return tensorTransformCustom;
}