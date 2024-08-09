# Depth estimation

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_depth_midas_v2_tflite.cpp](./cpp/example_depth_midas_v2_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | Midas v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>custom C++ decoding

NOTES:
* No GPU support on i.MX 93.
* Distances shown on screen are relative and not absolute from the camera.
* The whitest, the closest.

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### C++
C++ example script can be run after [cross compilation](../). To use NPU backend, use the following command, otherwise, look at the notes :
```bash
$ ./build/depth/example_depth_midas_v2_tflite -p /path/to/nxp-nnstreamer-examples/downloads/models/depth/midas_2_1_small_int8_quant.tflite
```
NOTES:
* For i.MX 93 use vela model.
* For CPU backend, add : -b CPU.
* For GPU backend, add : -b GPU.

The following execution parameters are available (Run ``` ./example_depth_midas_v2_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path | Use the selected model path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.