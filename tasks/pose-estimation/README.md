# Pose Estimation

## Overview
Name | Implementation | Model | ML engine |Features
--- | --- | --- | --- | ---
[example_pose_movenet_tflite.py](./example_pose_movenet_tflite.py) | Python | MoveNet Lightning | TFLite | v4l2/libcamera<br>video file decoding<br>gst-launch<br>custom model decoding
[example_pose_movenet_tflite.cpp](./cpp/example_pose_movenet_tflite.cpp) | C++ | MoveNet Lightning | TFLite | v4l2/libcamera<br>video file decoding<br>gst-launch<br>custom model decoding

## MoveNet Lightning pose estimation
### Python Execution

Default backend can be overridden by explicitly defining BACKEND variable, and source can be selected as VIDEO or CAMERA.
Similarly, the GPU variable allows to choose between 2D GPU (GPU2D) or 3D GPU (GPU3D) if available for scaling and color space conversion operations.
For instance:

```bash
BACKEND=NPU SOURCE=CAMERA GPU=GPU2D ./tasks/pose-estimation/example_pose_movenet_tflite.py
```

By default:
- SOURCE is set to VIDEO
- BACKEND is set to CPU
- GPU is set to GPU2D
- If the board does not support video decoding, the application will fallback to CAMERA source instead
- Camera resolution is set to 640x480

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite.py -h``` to see option details):

Option | Description
--- | ---
--video_file/-f FILE | Selects another video source file with given FILE path
--video_dims/-d WIDTH HEIGHT | Provides the video source resolution
--mirror/-m | Flips the camera stream when using a front camera
--camera_device/-c CAMERA_DEVICE | camera device node
--no-square_cropping | resize preserving video ratio instead of video cropping

Note: Video used is in Matroska (.mkv) format with VP9 encoding, which is not supported by i.MX 9 boards decoding capabilities

### C++ Execution

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

#### NPU Inference

For i.MX 8M Plus (VSI NPU):
```bash
./build/pose-estimation/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO}
```

#### Inferences on other hardwares

Inference on CPU with the following script:
```bash
./build/pose-estimation/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO} -b CPU
```
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/pose-estimation/example_pose_movenet_tflite -p ${MOVENET_QUANT} -f ${POWER_JUMP_VIDEO} -b GPU
```

#### C++ Execution Parameters

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: CPU
-n, --normalization | Use the selected normalization (none, centered, scaled, centeredScaled)<br> default: none
-p, --model_path | Use the selected model path
-f, --video_file | Use the selected video file instead of camera source
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory
-r, --cam_params | Use the selected camera resolution and framerate<br> default: 640x480, 30fps
-u, --use_gpu3d  | Use the 3D GPU hardware acceleration for video transformation (if available)<br> default: false

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
