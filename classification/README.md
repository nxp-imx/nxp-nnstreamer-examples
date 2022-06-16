# Image classification

## Overview
Name | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | ---
[example_classification_mobilenet_v1_tflite.sh](./example_classification_mobilenet_v1_tflite.sh) | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_mobilenet_v1_dvrt.sh](./example_classification_mobilenet_v1_dvrt.sh) | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | DeepViewRT | NPU (defaut)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93
* DeepVIewRT support is restricted to CPU on i.MX 93

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
```
# BACKEND=CPU ./classification/example_classification_mobilenet_v1_tflite.sh
```