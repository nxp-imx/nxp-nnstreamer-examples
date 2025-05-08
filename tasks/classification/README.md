# Image Classification

## Overview
Name | Implementation | Model | ML engine | Features
--- | --- | --- | --- | --- |
[example_classification_mobilenet_v1_tflite.cpp](./cpp/example_classification_mobilenet_v1_tflite.cpp) | C++ | MobileNetV1 | TFLite| camera<br>gst-launch<br>
[example_classification_mobilenet_v1_tflite.sh](./example_classification_mobilenet_v1_tflite.sh) | Bash | MobileNetV1 | TFLite| camera<br>gst-launch<br>

## MobileNetV1 image classification
### Bash
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :white_check_mark: | :white_check_mark: | :x: |

The image classification demo in bash supports multiple backend (refers to above table), default value can be overriden by explicitly defining BACKEND variable, for instance:
```bash
BACKEND=CPU ./tasks/classification/example_classification_mobilenet_v1_tflite.sh
```

### C++
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :white_check_mark: | :white_check_mark: | :white_check_mark: |

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

Image classification demo can be run on different hardware (refers to above table):<br>
Inference on NPU with the following script:
```bash
./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT} -l ${MOBILENETV1_LABELS}
```
For i.MX 93 NPU use vela converted model:
```bash
./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT_VELA} -l ${MOBILENETV1_LABELS}
```

For i.MX 95 NPU use neutron converted model:
```bash
./build/classification/example_classification_mobilenet_v1_tflite -p  ${MOBILENETV1_QUANT_NEUTRON} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```

Inference on CPU with the following script:
```bash
./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1_QUANT} -l ${MOBILENETV1_LABELS} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/classification/example_classification_mobilenet_v1_tflite -p ${MOBILENETV1} -l ${MOBILENETV1_LABELS} -b GPU -n centeredScaled
```
Input normalization needs to be specified, here input data needs to be centered and scaled to fit MobileNetV1 input specifications.

The following execution parameters are available (Run ``` ./example_classification_mobilenet_v1_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, scaled, centeredScaled, castInt32, castuInt8)<br> default: none
-c, --camera_device | Use the selected camera device (/dev/video{number})<br>default: /dev/video0 for i.MX 93 and /dev/video3 for i.MX 8MP
-p, --model_path | Use the selected model path
-l, --labels_path | Use the selected labels path
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory
-r, --cam_params | Use the selected camera resolution and framerate<br> default: 640x480, 30fps

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.<br><br>