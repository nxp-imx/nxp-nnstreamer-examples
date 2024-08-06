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
C++ example script can be run after [cross compilation](../). To use CPU backend, use the following command, otherwise, look at the notes :
```bash
$ export MODEL_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/pose"
$ export MODEL_MEDIA="/path/to/nxp-nnstreamer-examples/downloads/media/movies"
$ ./build/pose/example_pose_movenet_tflite -p ${MODEL_PATH}/movenet_single_pose_lightning.tflite -f ${MODEL_MEDIA}/Conditioning_Drill_1-_Power_Jump.webm.480p.vp9.webm
```
NOTES:
* For NPU backend, use movenet_quant.tflite model or vela model for i.MX 93, and add : -b NPU -n none.

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: castInt32
-p, --model_path | Use the selected model path
-f, --video_file | Use the selected video file
-u, --use_camera | If we use camera or video input (true,false)<br> default: false (true for i.MX 93)
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
