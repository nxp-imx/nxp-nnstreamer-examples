# Depth estimation

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_depth_midas_v2_tflite.cpp](./cpp/example_depth_midas_v2_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | Midas v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>custom C++ decoding

NOTES:
* No GPU support on i.MX 93
* Distances shown on screen are relative and not absolute from the camera
* The whitest, the closest

## Execution
### C++
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
It is possible to run the depth demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
$ ./build/depth/example_depth_midas_v2_tflite -p ${MIDASV2}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/depth/example_depth_midas_v2_tflite -p ${MIDASV2_VELA}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

Inference on CPU with the following script:
```bash
$ ./build/depth/example_depth_midas_v2_tflite -p ${MIDASV2} -b CPU
```
NOTE: Inference on i.MX8MPlus GPU is possible but not recommended because of low performances:
```bash
$ ./build/depth/example_depth_midas_v2_tflite -p ${MIDASV2} -b GPU
```
The following execution parameters are available (Run ``` ./example_depth_midas_v2_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path | Use the selected model path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.