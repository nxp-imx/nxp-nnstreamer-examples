# Pose detection

## Overview
Name | Implementation | Model | ML engine |Features
--- | --- | --- | --- | ---
[example_pose_movenet_tflite.py](./example_pose_movenet_tflite.py) | Python | MoveNet Lightning | TFLite | video file decoding (i.MX 8M Plus only)<br>camera<br>gst-launch<br>
[example_pose_movenet_tflite.cpp](./cpp/example_pose_movenet_tflite.cpp) | C++ | MoveNet Lightning | TFLite | video file decoding (i.MX 8M Plus only)<br>camera<br>gst-launch<br>

## MoveNet Lightning pose detection
### Python
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :x: | :white_check_mark: | :x: |

Default backend can be overriden by explicitly defining BACKEND variable, and source can be selected as VIDEO or CAMERA, for instance:

```bash
BACKEND=NPU SOURCE=CAMERA ./pose/example_pose_movenet_tflite.py
```

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite.py -h``` to see option details):

Option | Description
--- | ---
--video_file FILE | Selects another video source file with given FILE path
--video_dims WIDTH HEIGHT | Provides the video source resolution
--mirror | Flips the camera stream when using a front camera

### C++
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :x: | :white_check_mark: | :white_check_mark: |

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

It is possible to run the pose detection demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO}
```
For i.MX 93 use vela converted model:
```bash
./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT_VELA} -f ${POWER_JUMP_VIDEO}
```
Inference on CPU with the following script:
```bash
./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO} -b CPU
```
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/pose/example_pose_movenet_tflite -p ${MOVENET} -f ${POWER_JUMP_VIDEO} -b GPU -n castInt32
```
The following execution parameters are available (Run ``` ./example_pose_movenet_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: CPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: castuInt8
-p, --model_path | Use the selected model path
-f, --video_file | Use the selected video file
-u, --use_camera | If we use camera or video input (true,false)<br> default: false (true for i.MX 93)
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory
-r, --cam_params | Use the selected camera resolution and framerate<br> default: 640x480, 30fps

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
