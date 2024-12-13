# Object detection

## Overview
Name | Implementation | Model | ML engine | Features
--- | --- | --- | --- | --- |
[example_detection_mobilenet_ssd_v2_tflite.cpp](./cpp/example_detection_mobilenet_ssd_v2_tflite.cpp) | C++ | SSD MobileNetV2 | TFLite | camera<br>gst-launch<br>
[example_detection_mobilenet_ssd_v2_tflite.sh](./example_detection_mobilenet_ssd_v2_tflite.sh) | Bash | SSD MobileNetV2 | TFLite | camera<br>gst-launch<br>
[example_detection_yolo_v4_tiny_tflite.sh](./example_detection_yolo_v4_tiny_tflite.sh) | Bash | YOLOv4 Tiny | TFLite | camera<br>gst-launch<br>[custom python tensor_filter](./postprocess_yolov4_tiny.py)

## YOLOv4 Tiny object detection 
### Bash
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :x: | :x: | :x: |

*NOTE: YOLOv4 Tiny output does not directly work with the YOLOv5 mode of tensor_decoder element, so a python filter is used to post-process and reshape this output as required.*

The object detection demo in bash using YOLOv4 Tiny supports multiple backend (refers to above table), default value can be overriden by explicitly defining BACKEND variable, for instance:
```bash
BACKEND=CPU ./detection/example_detection_yolo_v4_tiny_tflite.sh
```

## SSD MobileNetV2 object detection
### Bash
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :white_check_mark: | :white_check_mark: | :x: |

*NOTE: A warmup time for NPU inference on i.MX 95 can take up to 1 minute.*

The object detection demo in bash using SSD MobiletNetV2 supports multiple backend (refers to above table), default value can be overriden by explicitly defining BACKEND variable, for instance:
```bash
BACKEND=CPU ./detection/example_detection_mobilenet_ssd_v2_tflite.sh
```
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

It is possible to run the object detection demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
For i.MX 93 use vela converted model:
```bash
./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT_VELA} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```

Inference on CPU with the following script:
```bash
./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES} -b GPU -n centeredReduced
```
The following execution parameters are available (Run ``` ./example_detection_mobilenet_ssd_v2_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path | Use the selected model path
-l, --labels_path | Use the selected labels path
-x, --boxes_path | Use the selected boxes path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.