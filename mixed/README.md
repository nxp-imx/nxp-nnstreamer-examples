# Mixed examples

## Overview
Name | Implementation | Model | ML engine | Features
--- | --- | --- | --- | ---
[example_classification_and_detection_tflite.cpp](./cpp/example_classification_and_detection_tflite.cpp) | C++ | MobileNetV1	<br> SSD MobileNetV2 | TFLite | camera<br>gst-launch<br>
[example_face_and_pose_detection_tflite.cpp](./cpp/example_face_and_pose_detection_tflite.cpp) | C++ | UltraFace <br> MoveNet Lightning	 | TFLite | camera<br>gst-launch<br>
[example_emotion_and_detection_tflite.cpp](./cpp/example_emotion_and_detection_tflite.cpp) | C++ | UltraFace <br> Deepface-emotion <br> SSD MobileNetV2 | TFLite | camera<br>video file decoding<br>gst-launch<br>

Mixed examples goal is to demonstrate the possibility to make applications which use multiple models running in parallel, while keeping good performances. Three examples are available:

## Image classification and object detection (MobileNetV1/SSD MobileNetV2)
### C++
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :white_check_mark: | :white_check_mark: | :white_check_mark: |

*NOTE: A warmup time for NPU inference on i.MX 95 can take up to 1 minute.*

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

An example running image classification and object detection inferences in parallel is provided, which use the video saving feature. This example can be run with both models on NPU, using the following command:
 ```bash
./build/mixed/example_classification_and_detection_tflite -p ${MOBILENETV1_QUANT},${MOBILENETV2_QUANT} -l ${MOBILENETV1_LABELS},${COCO_LABELS} -x ${MOBILENETV2_BOXES} -f ${SAVE_VIDEO_PATH}
```
For i.MX 93 use vela converted model:<br>
NOTE: Video saving feature is not available on i.MX 93.
 ```bash
./build/mixed/example_classification_and_detection_tflite -p ${MOBILENETV1_QUANT_VELA},${MOBILENETV2_QUANT_VELA} -l ${MOBILENETV1_LABELS},${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
To use CPU or GPU backend, refers to the execution parameter ```--backend``` below.<br>
To use the non-quantized (float32) model, input normalization needs to be set with the execution parameter ```--normalization``` (description below): For MobileNetV1 and MobiletNetV2 use ```centeredReduced``` argument. Use ```MOBILENETV1``` or ```MOBILENETV2``` environment variable for model path.<br>
The following execution parameters are available (Run ``` ./example_classification_and_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend CLASS_BACKEND,DET_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,NPU
-n, --normalization CLASS_NORM,DET_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8M Plus
-p, --model_path CLASS_MODEL,DET_MODEL | Use the selected model path
-l, --labels_path CLASS_LABELS,DET_LABELS | Use the selected labels path
-x, --boxes_path DET_BOXES | Use the selected boxes path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory
-f, --video_file | Use the selected path to generate a video (only for i.MX 8M Plus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

## Pose detection and face detection (MoveNet/UltraFace)
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

An example running face detection and pose detection inferences in parallel is available. This example can be run with both models on NPU, using the following command:
```bash
./build/mixed/example_face_and_pose_detection_tflite -p ${ULTRAFACE_QUANT},${MOVENET_QUANT}
```
For i.MX 93 use vela converted model:
```bash
./build/mixed/example_face_and_pose_detection_tflite -p ${ULTRAFACE_QUANT_VELA},${MOVENET_QUANT_VELA}
```
To use CPU or GPU backend, refers to the execution parameter ```--backend``` below.<br>
To use the non-quantized (float32) MoveNet model, input normalization needs to be set with the execution parameter ```--normalization``` (description below) to ```castInt32``` and use ```MOVENET``` environment variable for model path.<br>
The following execution parameters are available (Run ``` ./example_face_and_pose_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend FACE_BACKEND,POSE_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,NPU
-n, --normalization FACE_NORM,POSE_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,castuInt8
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path FACE_MODEL,POSE_MODEL | Use the selected model path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.

## Emotion detection and object detection (UltraFace/Deepface-emotion/SSD MobileNetV2)
### C++
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :x: | :x: | :x: |
|   i.MX 95    | :x: | :white_check_mark: | :white_check_mark: |

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

An example running emotion detection and object detection inferences in parallel is available. This example can be run with both models on NPU, using the following command:
```bash
./build/mixed/example_emotion_and_detection_tflite -p ${ULTRAFACE_QUANT},${EMOTION_QUANT},${MOBILENETV2_QUANT} -f ${POWER_JUMP_VIDEO} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
To use CPU or GPU backend, refers to the execution parameter ```--backend``` below.<br>
To use the non-quantized (float32) MobileNetV2 model, input normalization needs to be set with the execution parameter ```--normalization``` (description below) to ```centeredReduced``` and use ```MOBILENETV2``` environment variable for model path.<br>
The following execution parameters are available (Run ``` ./example_emotion_and_detection_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend FACE_BACKEND,EMOTION_BACKEND,DETECTION_BACKEND | Use the selected backend (CPU, GPU, NPU)<br> default: NPU,CPU
-n, --normalization FACE_NORM,EMOTION_NORM,DETECTION_NORM | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none,castInt32
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path FACE_MODEL,EMOTION_MODEL,DETECTION_MODEL | Use the selected model path
-f, --video_file | Use the selected video file
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.