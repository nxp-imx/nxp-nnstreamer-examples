# Segmentation

## Overview
Name | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | ---
[example_segmentation_deeplab_v3_tflite.sh](./example_segmentation_deeplab_v3_tflite.sh) | i.MX 8M Plus | deeplab_v3_mnv2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | multifilesrc<br>gst-launch<br>

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
```
# BACKEND=CPU ./example_segmentation_deeplab_v3_tflite.sh
```