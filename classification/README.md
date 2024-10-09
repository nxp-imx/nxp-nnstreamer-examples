# Image classification

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_classification_mobilenet_v1_tflite.cpp](./cpp/example_classification_mobilenet_v1_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_two_cameras_tflite.cpp](./cpp/example_classification_two_cameras_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_mobilenet_v1_tflite.sh](./example_classification_mobilenet_v1_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93.

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./classification/example_classification_mobilenet_v1_tflite.sh
```
### C++
C++ example script can be run after [cross compilation](../). To use NPU backend, use the following command, otherwise, look at the notes :
```bash
$ export MODEL_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/classification"
$ ./build/classification/example_classification_mobilenet_v1_tflite -p ${MODEL_PATH}/mobilenet_v1_1.0_224_quant_uint8_float32.tflite -l ${MODEL_PATH}/labels_mobilenet_quant_v1_224.txt
```
NOTES:
* For i.MX 93 use vela model.
* For CPU backend, use mobilenet_v1_1.0_224.tflite model and add : -b CPU -n centeredReduced.
* For GPU backend, use mobilenet_v1_1.0_224.tflite model and add : -b GPU -n centeredReduced.

The following execution parameters are available (Run ``` ./example_classification_mobilenet_v1_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path | Use the selected model path
-l, --labels_path | Use the selected labels path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

An example using two cameras is available. To use CPU backend on first camera, and NPU backend on second camera, use the following command, otherwise, look at the notes :
```bash
$ export MODEL_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/classification"
$ ./build/classification/example_classification_two_cameras_tflite -p ${MODEL_PATH}/mobilenet_v1_1.0_224.tflite,${MODEL_PATH}/mobilenet_v1_1.0_224_quant_uint8_float32.tflite -l ${MODEL_PATH}/labels_mobilenet_quant_v1_224.txt -c /dev/video3,/dev/video5 -b CPU,NPU -n centeredReduced,none
```
NOTES:
* For i.MX 93 use vela model.
* For CPU backend, use mobilenet_v1_1.0_224.tflite model and add : -b CPU -n centeredReduced.
* For GPU backend, use mobilenet_v1_1.0_224.tflite model and add : -b GPU -n centeredReduced.

The following execution parameters are available (Run ``` ./example_classification_two_cameras_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend CAM1_BACKEND,CAM2_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization CAM1_NORM,CAM2_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-c, --camera_device CAMERA1,CAMERA2 | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path CAM1_MODEL,CAM2_MODEL | Use the selected model path
-l, --labels_path CAM1_LABELS,CAM2_LABELS | Use the selected labels path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.