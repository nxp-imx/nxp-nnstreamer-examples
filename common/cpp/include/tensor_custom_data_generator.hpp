/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */

#ifndef CPP_TENSOR_CUSTOM_DATA_GENERATOR_H_
#define CPP_TENSOR_CUSTOM_DATA_GENERATOR_H_

#include <map>
#include <string>
#include <filesystem>
#include "imx_devices.hpp"


/**
 * @brief Data structure for tensor.
 */
typedef struct {
  std::string tensorFilterCustom;
  std::string tensorTransform;
} TensorData;


/**
 * @brief Data structure for labels and boxes directories.
 */
typedef struct {
  std::filesystem::path labelsDir;
  std::filesystem::path boxesDir; 
} DataDir;


/** 
 * @brief Normalization enumeration.
 */
enum class Normalization {
  none,
  centered,
  reduced,
  centeredReduced,
  castInt32,
  castuInt8,
};


/**
 * @brief Check if the element exists in the dictionary. 
 * 
 * @param element: element we want to check.
 * @param dictionary: dictionary we want to check.
 * @return the selected element or a default one.
 */
template <typename T>
T selectFromDictionary(const std::string &element, 
                         std::map<std::string, T> dictionary)
{
  for (auto &pair : dictionary) {
    std::string key = pair.first;
    if (element == key)
      return dictionary[element];
  }
  if constexpr(std::is_same_v<T, Normalization>)
    return T::none;
  if constexpr(std::is_same_v<T, imx::Backend>)
    return T::NPU;
}


/** 
 * @brief Dictionary of Normalization identification.
 */ 
extern std::map<std::string, Normalization> normDictionary;


/**
 * @brief Create pipeline segments for customized tensor data and
 *        set up USE_GPU_INFERENCE, an environment variable used
 *        for the GPU backend on i.MX 8M Plus.
 */
class TensorCustomGenerator {
  private:
    TensorData tensorData;

  public:
    std::string CPU(const int &numThreads);

    std::string vsiGPU();

    std::string vsiNPU();

    std::string ethosNPU();

    std::string neutronNPU();

    std::string GPU();

    std::string setTensorTransformConfig(const std::string &norm);
};
#endif