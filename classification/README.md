# Image classification

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_classification_mobilenet_v1_tflite.cpp](./cpp/example_classification_mobilenet_v1_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_two_cameras_tflite.cpp](./cpp/example_classification_two_cameras_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_classification_mobilenet_v1_tflite.sh](./example_classification_mobilenet_v1_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95 | mobilenet_v1 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* Warmup time for NPU inference on i.MX 95 can take up to 1 minute
* No GPU support on i.MX 93

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./classification/example_classification_mobilenet_v1_tflite.sh
```
### C++
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
It is possible to run the classification demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
$ ./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT} -l ${MOBILENETV1_LABELS}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT_VELA} -l ${MOBILENETV1_LABELS}
```
Inference on CPU with the following script:
```bash
$ ./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT} -l ${MOBILENETV1_LABELS} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
$ ./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1} -l ${MOBILENETV1_LABELS} -b GPU -n centeredReduced
```
Input normalization needs to be specified, here input data needs to be centered and reduced to fit MobileNetv1 input specifications.

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

An example using two cameras is available. To NPU backend on on both camera inference, use the following command:<br>
NOTES:
- it is not recommanded to use CPU or GPU bachend because of low performances.
- performances may depend on the USB cameras used, especially on i.MX 93.
```bash
$ ./build/classification/example_classification_two_cameras_tflite -p ${MOBILENETV1_QUANT},${MOBILENETV1_QUANT} -l ${MOBILENETV1_LABELS} -c ${CAM1_PATH},${CAM2_PATH}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/classification/example_classification_two_cameras_tflite -p ${MOBILENETV1_QUANT_VELA},${MOBILENETV1_QUANT_VELA} -l ${MOBILENETV1_LABELS} -c ${CAM1_PATH},${CAM2_PATH}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.<br><br>
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
