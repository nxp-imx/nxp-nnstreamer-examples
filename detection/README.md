# Object detection

## Overview
Name | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | ---
[example_detection_mobilenet_ssd_v2_tflite.sh](./example_detection_mobilenet_ssd_v2_tflite.sh) | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_mobilenet_ssd_v2_dvrt.sh](./example_detection_mobilenet_ssd_v2_dvrt.sh) | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | DeepViewRT | NPU (defaut)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93
* DeepVIewRT support is restricted to CPU on i.MX 93

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
```
# BACKEND=CPU ./detection/example_detection_mobilenet_ssd_v2_tflite.sh
```