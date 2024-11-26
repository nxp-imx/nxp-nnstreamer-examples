# Pose detection

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_pose_movenet_tflite.py](./example_pose_movenet_tflite.py) | Python | i.MX 8M Plus <br> i.MX 93| PoseNet Lightning | TFLite | CPU<br>NPU | video file decoding (i.MX 8M Plus only)<br>camera<br>gst-launch<br>
[example_pose_movenet_tflite.cpp](./cpp/example_pose_movenet_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93| PoseNet Lightning | TFLite | CPU<br>NPU | video file decoding (i.MX 8M Plus only)<br>camera<br>gst-launch<br>

## Execution
Example script can be called from target console with no further restriction.
Default backend can be overriden by explicitly defining BACKEND variable, and source can be selected as VIDEO or CAMERA, for instance:
### Python
```bash
$ BACKEND=NPU SOURCE=CAMERA ./pose/example_pose_movenet_tflite.py
```

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite.py -h``` to see option details):

Option | Description
--- | ---
--video_file FILE | Selects another video source file with given FILE path
--video_dims WIDTH HEIGHT | Provides the video source resolution
--mirror | Flips the camera stream when using a front camera

## C++
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
It is possible to run the pose detection demo inference on two different hardwares:<br>
Inference on NPU with the following script:
```bash
$ ./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT_VELA} -f ${POWER_JUMP_VIDEO}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

Inference on CPU with the following script:
```bash
$ ./build/pose/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO} -b CPU
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
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
