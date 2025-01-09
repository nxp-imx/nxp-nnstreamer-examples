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
  {"reduced", Normalization::reduced},
  {"centeredReduced", Normalization::centeredReduced},
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
std::string TensorCustomGenerator::setTensorTransformConfig()
{
  std::string tensorTransformCustom;
  switch (selectFromDictionary(norm, normDictionary))
  {
    case Normalization::none:
      tensorTransformCustom = "";
      break;

    case Normalization::centered:
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += "option=typecast:int16,add:-128 ! ";
      tensorTransformCustom += "tensor_transform mode=typecast ";
      tensorTransformCustom += "option=int8 ! ";
      break;

    case Normalization::reduced:
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += "option=typecast:float32,div:255 ! ";
      break;

    case Normalization::centeredReduced:
      tensorTransformCustom = "tensor_transform mode=arithmetic ";
      tensorTransformCustom += "option=typecast:float32,add:-127.5,div:127.5 ! ";
      break;

    case Normalization::castInt32:
      tensorTransformCustom = "tensor_transform mode=typecast ";
      tensorTransformCustom += "option=int32 ! ";
      break;

    case Normalization::castuInt8:
      tensorTransformCustom = "tensor_transform mode=typecast ";
      tensorTransformCustom += "option=uint8 ! ";
      break;

    default:
      tensorTransformCustom = "";
      break;
  }
  return tensorTransformCustom;
}