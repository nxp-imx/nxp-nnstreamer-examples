# Mixed examples

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_classification_and_detection_tflite.cpp](./cpp/example_classification_and_detection_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_v1	<br> mobilenet_ssd_v2 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_face_and_pose_detection_tflite.cpp](./cpp/example_face_and_pose_detection_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | UltraFace <br> PoseNet Lightning	 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_emotion_and_detection_tflite.cpp](./cpp/example_emotion_and_detection_tflite.cpp) | C++ | i.MX 8M Plus | UltraFace <br> Deepface-emotion <br> mobilenet_ssd_v2 | TFLite | NPU<br>GPU<br>CPU<br> | camera<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93.
 
## Execution
### Classification and object detection in parallel, with video saving (C++)
C++ example script can be run after [cross compilation](../).<br>
Example of classification and object detection in parallel can be run with both models on NPU, using the following command :
 ```bash
$ export CLASS_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/classification"
$ export DET_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/detection"
$ ./build/mixed/example_classification_and_detection_tflite -p ${CLASS_PATH}/mobilenet_v1_1.0_224_quant_uint8_float32.tflite,${DET_PATH}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess.tflite -l ${CLASS_PATH}/labels_mobilenet_quant_v1_224.txt,${DET_PATH}/coco_labels_list.txt -x ${DET_PATH}/box_priors.txt -f path/to/save/video.mkv
```
To use other backends, refers to classification and object detection README for further details.<br>
The output can't be saved on i.MX 93. <br>
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

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

### Pose detection and face detection in parallel (C++)
Example of face and pose detection in parallel can be run with face detection on NPU, and pose detection on CPU, using the following command :
```bash
$ export FACE_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/face"
$ export POSE_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/pose"
$ ./build/mixed/example_face_and_pose_detection_tflite -p ${FACE_PATH}/ultraface_slim_uint8_float32.tflite,${POSE_PATH}/movenet_single_pose_lightning.tflite
```
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
Example of emotion and object detection in parallel can be run on NPU, using the following command :
```bash
$ export EMOTION_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/face"
$ export DET_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/detection"
$ export MEDIA_PATH="/path/to/nxp-nnstreamer-examples/downloads/media/movies"
$ ./build/mixed/example_emotion_and_detection_tflite -p ${EMOTION_PATH}/ultraface_slim_uint8_float32.tflite,${EMOTION_PATH}/emotion_uint8_float32.tflite,${DET_PATH}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess.tflite -f ${MEDIA_PATH}/slow_traffic_small.mkv -l ${DET_PATH}/coco_labels_list.txt -x ${DET_PATH}/box_priors.txt
```
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