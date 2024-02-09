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

The following execution parameters are available (Run ``` ./example_pose_movenet_tflite.py -h``` to see options detail):

Option | Description
--- | ---
--video_file FILE | Selects another video source file with given FILE path
--video_dims WIDTH HEIGHT | Provides the video source resolution
--mirror | Flips the camera stream when using a front camera

Esc or ctrl+C key presses stop the execution of the pipeline.
