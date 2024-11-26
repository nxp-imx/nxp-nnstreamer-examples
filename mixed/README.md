# Mixed examples

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_classification_and_detection_tflite.cpp](./cpp/example_classification_and_detection_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1	<br> mobilenet_ssd_v2 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_face_and_pose_detection_tflite.cpp](./cpp/example_face_and_pose_detection_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | UltraFace <br> PoseNet Lightning	 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_emotion_and_detection_tflite.cpp](./cpp/example_emotion_and_detection_tflite.cpp) | C++ | i.MX 8M Plus | UltraFace <br> Deepface-emotion <br> mobilenet_ssd_v2 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93
 
## Execution
### Classification and object detection in parallel, with video saving (C++)
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
Example of classification and object detection in parallel can be run with both models on NPU, using the following command:
 ```bash
$ ./build/mixed/example_classification_and_detection_tflite -p ${MOBILENETV1_QUANT},${MOBILENETV2_QUANT} -l ${MOBILENETV1_LABELS},${COCO_LABELS} -x ${MOBILENETV2_BOXES} -f ${SAVE_VIDEO_PATH}
```
For i.MX 93 use vela converted model:
 ```bash
$ ./build/mixed/example_classification_and_detection_tflite -p ${MOBILENETV1_QUANT_VELA},${MOBILENETV2_QUANT_VELA} -l ${MOBILENETV1_LABELS},${COCO_LABELS} -x ${MOBILENETV2_BOXES} -f ${SAVE_VIDEO_PATH}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

To use other backends, refers to classification and object detection README for further details.<br>
The output can't be saved on i.MX 93.<br>
The following execution parameters are available (Run ``` ./example_classification_and_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend CLASS_BACKEND,DET_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,NPU
-n, --normalization CLASS_NORM,DET_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path CLASS_MODEL,DET_MODEL | Use the selected model path
-l, --labels_path CLASS_LABELS,DET_LABELS | Use the selected labels path
-x, --boxes_path DET_BOXES | Use the selected boxes path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)
-f, --video_file | Use the selected path to generate a video (optional)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

### Pose detection and face detection in parallel (C++)
Example of face and pose detection in parallel can be run with face detection and pose detection on NPU, using the following command:
```bash
$ ./build/mixed/example_face_and_pose_detection_tflite -p ${ULTRAFACE_QUANT},${MOVENET_QUANT}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/mixed/example_face_and_pose_detection_tflite -p ${ULTRAFACE_QUANT_VELA},${MOVENET_QUANT_VELA}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

To use other backends, refers to pose detection and face detection README for further details.<br>
The following execution parameters are available (Run ``` ./example_face_and_pose_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend FACE_BACKEND,POSE_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,CPU
-n, --normalization FACE_NORM,POSE_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,castInt32
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path FACE_MODEL,POSE_MODEL | Use the selected model path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

### Emotion detection and object detection in parallel (C++)
Example of emotion and object detection in parallel can be run on NPU, using the following command:
```bash
$ ./build/mixed/example_emotion_and_detection_tflite -p ${ULTRAFACE_QUANT},${EMOTION_QUANT},${MOBILENETV2_QUANT} -f ${POWER_JUMP_VIDEO} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/mixed/example_emotion_and_detection_tflite -p ${ULTRAFACE_QUANT_VELA},${EMOTION_QUANT_VELA},${MOBILENETV2_QUANT_VELA} -f ${POWER_JUMP_VIDEO} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

To use other backends, refers to pose detection and face detection README for further details.<br>
The following execution parameters are available (Run ``` ./example_emotion_and_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend FACE_BACKEND,EMOTION_BACKEND,DETECTION_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,CPU
-n, --normalization FACE_NORM,EMOTION_NORM,DETECTION_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,castInt32
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path FACE_MODEL,EMOTION_MODEL,DETECTION_MODEL | Use the selected model path
-f, --video_file | Use the selected video file
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.