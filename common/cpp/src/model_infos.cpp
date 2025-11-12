/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#include "model_infos.hpp"

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/tools/delegates/delegate_provider.h>
#include <tensorflow/lite/delegates/external/external_delegate.h>

/** 
 * @brief Dictionary of Backend identification.
 */ 
std::map<std::string, imx::Backend> inferenceHardwareBackend = {
  {"CPU", imx::Backend::CPU},
  {"GPU", imx::Backend::GPU},
  {"NPU", imx::Backend::NPU},
};


/**
 * @brief Parameterized constructor.
 * 
 * @param path: model path.
 * @param backend: second argument at runtime corresponding to backend use.
 * @param norm: normalization to apply to input data.
 * @param numThreads: number of threads for XNNPACK (CPU backend).
 */
ModelInfos::ModelInfos(const std::filesystem::path &path,
                       const std::string &backend,
                       const std::string &norm,
                       const int &numThreads)
    : modelPath(path), backend(backend), modelWidth(0), modelHeight(0), modelChannel(0)
{
  setTensorFilterConfig(imx, numThreads);
  tensorData.tensorNormalization = norm;
}


/**
 * @brief Create pipeline segment for inference.
 * 
 * @param pipeline: GstPipelineImx pipeline.
 * @param gstName: tensor_filter element name, empty by default.
 * @param format: tensor_filter input format, RGB by default.
 */
void ModelInfos::addInferenceToPipeline(GstPipelineImx &pipeline,
                                        const std::string &gstName,
                                        const std::string &format)
{
  std::string cmd;
  if (format == "RGB") {
    videoscale.videoscaleToRGB(pipeline, modelWidth, modelHeight);
  } else {
    videoscale.videoTransform(pipeline, format, modelWidth, modelHeight, false, false, true);
  }
  tensorData.tensorTransform = tensorCustomData.setTensorTransformConfig(tensorData.tensorNormalization, pipeline);
  cmd += "tensor_converter ! ";
  cmd += tensorData.tensorTransform;
  cmd += "tensor_filter latency=1 framework=" + framework + "  model=";
  cmd += modelPath.string() + " ";
  cmd += tensorData.tensorFilterCustom;
  if (gstName.length() != 0) {
    cmd += " name=" + gstName;
    pipeline.addFilterName(gstName);
  }
  cmd += " ! ";
  pipeline.addToPipeline(cmd);
}


/**
 * @brief Setup tensor configuration, select backend use and create video
 *        compositor segment pipeline.
 * 
 * @param imx: i.MX used.
 * @param numThreads: number of threads for XNNPACK (CPU backend).
 */
void ModelInfos::setTensorFilterConfig(imx::Imx &imx, const int &numThreads)
{
  switch (selectFromDictionary(backend, inferenceHardwareBackend))
  {
    case imx::Backend::CPU:
      tensorData.tensorFilterCustom = tensorCustomData.CPU(numThreads);
      break;

    case imx::Backend::GPU:
      if (imx.hasVsiGPU()) {
        tensorData.tensorFilterCustom = tensorCustomData.vsiGPU();
      } else if (imx.socId() == imx::IMX95) {
        tensorData.tensorFilterCustom = tensorCustomData.GPU();
      } else {
        log_error("can't used this backend with %s\n", imx.socName().c_str());
        exit(-1);
      }
      break;

    case imx::Backend::NPU:
      if (imx.isIMX8() && imx.hasNPU())
        tensorData.tensorFilterCustom = tensorCustomData.vsiNPU();

      if (imx.hasEthosNPU())
        tensorData.tensorFilterCustom = tensorCustomData.ethosNPU();

      if (imx.hasNeutronNPU())
        tensorData.tensorFilterCustom = tensorCustomData.neutronNPU();

      break;

    default:
      if (imx.isIMX8() && imx.hasNPU())
        tensorData.tensorFilterCustom = tensorCustomData.vsiNPU();

      if (imx.hasEthosNPU())
        tensorData.tensorFilterCustom = tensorCustomData.ethosNPU();

      if (imx.hasNeutronNPU())
        tensorData.tensorFilterCustom = tensorCustomData.neutronNPU();

      break;
  }
}


/**
 * @brief Parameterized constructor.
 * 
 * @param path: TFlite model path.
 * @param backend: second argument at runtime corresponding to backend use.
 * @param norm: normalization to apply to input data.
 */
TFliteModelInfos::TFliteModelInfos(const std::filesystem::path &path,
                                   const std::string &backend,
                                   const std::string &norm,
                                   const int &numThreads) 
                  : ModelInfos(path, backend, norm, numThreads)
{
if (modelPath.extension() == ".tflite") {
    framework = "tensorflow-lite";
    std::unique_ptr<tflite::FlatBufferModel> model;

    /* Load Model. */
    model = tflite::FlatBufferModel::BuildFromFile(modelPath.string().c_str());
    if (model == nullptr) {
      fprintf(stderr, "Failed to load model\n");
      exit(-1);
    }

    /* Initiate Interpreter. */
    std::unique_ptr<tflite::Interpreter> interpreter;
    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder(*model.get(), resolver)(&interpreter);
    if (interpreter == nullptr) {
      log_error("Failed to initiate the interpreter\n");
      exit(-1);
    }

    /* Add Ethos delegate if we use imx93 NPU. */
    if (backend == "NPU" and imx.hasEthosNPU()) {     
      const char* delegateDir = "/usr/lib/libethosu_delegate.so";
      auto delegateOptions = TfLiteExternalDelegateOptionsDefault(delegateDir);
      auto externalDelegate = TfLiteExternalDelegateCreate(&delegateOptions);

      auto delegate = tflite::Interpreter::TfLiteDelegatePtr(
                          externalDelegate, [](TfLiteDelegate* delegate) {
                          TfLiteExternalDelegateDelete(delegate); }
                          );

      if(interpreter->ModifyGraphWithDelegate(std::move(delegate)) != kTfLiteOk) {
        log_error("Failed to apply delegate\n");
        exit(-1);
      }

      if (interpreter->AllocateTensors() != kTfLiteOk) {
        log_error("Failed to allocate tensor\n");
        exit(-1);
      }
    }

    /* Get input tensor dimensions. */
    int input = interpreter->inputs()[0];
    modelHeight = interpreter->tensor(input)->dims->data[1];
    modelWidth = interpreter->tensor(input)->dims->data[2];
    modelChannel = interpreter->tensor(input)->dims->data[3];
  } else {
    log_error("TFlite model needed\n");
    exit(-1);
  }
}