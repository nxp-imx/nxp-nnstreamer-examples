# Monocular Depth Estimation

## Overview
Name |Platforms | Model | ML engine | Features
--- | --- | --- | --- | ---
[example_depth_midas_v2_tflite.cpp](./cpp/example_depth_midas_v2_tflite.cpp) | C++ | MiDaS v2 | TFLite | v4l2/libcamera<br>video file decoding<br>gst-launch<br>custom model decoding

## MiDaS v2 monocular depth estimation
### C++ Execution

*NOTES:*
* *Distances shown on screen are relative and not absolute from the camera*
* *The whitest, the closest*

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

#### NPU Inference

For i.MX 8M Plus (VSI NPU):
```bash
./build/monocular-depth-estimation/example_depth_midas_v2_tflite -p ${MIDASV2}
```

For i.MX 93 (Ethos-U65):
```bash
./build/monocular-depth-estimation/example_depth_midas_v2_tflite -p ${MIDASV2_VELA}
```

#### Inferences on other hardwares

Inference on CPU with the following script:
```bash
./build/monocular-depth-estimation/example_depth_midas_v2_tflite -p ${MIDASV2} -b CPU
```
NOTE: Inference on i.MX8MPlus GPU is possible but not recommended because of low performances:
```bash
./build/monocular-depth-estimation/example_depth_midas_v2_tflite -p ${MIDASV2} -b GPU
```

#### C++ Execution Parameters

The following execution parameters are available (Run ``` ./example_depth_midas_v2_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, scaled, centeredScaled)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-f, --video_file | Use the selected video file instead of camera source
-p, --model_path | Use the selected model path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory
-r, --cam_params | Use the selected camera resolution and framerate<br> default: 640x480, 30fps

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.