# Pose detection

## Overview
Name | Platforms | Model | ML engine | Backend    | Features
--- | --- | --- | --- |------------| ---
[example_pose_movenet_tflite.py](./example_pose_movenet_tflite.py) | i.MX 8M Plus | PoseNet Lightning | TFLite | CPU<br>NPU | video file decoding<br>camera<br>gst-launch<br>python<br>

## Execution
Example script can be called from target console with no further restriction.
Default backend can be overriden by explicitly defining BACKEND variable, and source can be selected as VIDEO or CAMERA, for instance:

```
# BACKEND=NPU SOURCE=CAMERA ./example_pose_movenet_tflite.py
```
Another source video can be chosen by giving its path to the --video_file option.<br>
Esc or ctrl+C key presses stop the execution of the pipeline.
