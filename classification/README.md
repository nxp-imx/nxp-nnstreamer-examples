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

It is also possible to choose the camera device with the C++ example.
```bash
# Set the path to classification data in CLASSIFICATION_DATA_PATH variable (default location : /tmp/models)
$ export CLASSIFICATION_DATA_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/classification/"
$ ./example_classification_mobilenet_v1_tflite -b NPU -c /dev/video3
```