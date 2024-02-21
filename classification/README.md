# Image classification

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_classification_mobilenet_v1_tflite.cc](./example_classification_mobilenet_v1_tflite.cc) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_mobilenet_v1_tflite.sh](./example_classification_mobilenet_v1_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./classification/example_classification_mobilenet_v1_tflite.sh
```
### C++
```bash
# Set the path to classification data in CLASSIFICATION_DATA_PATH variable (default location : /tmp/models)
$ export CLASSIFICATION_DATA_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/classification/"
$ ./example_classification_mobilenet_v1_tflite
```
The following execution parameters are available (Run ``` ./example_classification_mobilenet_v1_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP


Press ```Esc or ctrl+C``` to stop the execution of the pipeline.