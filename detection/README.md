# Object detection

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_detection_mobilenet_ssd_v2_tflite.cpp](./cpp/example_detection_mobilenet_ssd_v2_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95 | mobilenet_ssd_v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_mobilenet_ssd_v2_tflite.sh](./example_detection_mobilenet_ssd_v2_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_yolo_v4_tiny_tflite.sh](./example_detection_yolo_v4_tiny_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | yolov4_tiny | TFLite | NPU (default)<br>CPU<br> | camera<br>gst-launch<br>[custom python tensor_filter](./postprocess_yolov4_tiny.py)

NOTES:
* No GPU support on i.MX 93
* Yolov4-tiny output does not directly work with the Yolov5 mode of tensor_decoder element, so a python filter is used to post-process and reshape this output as required
 
## Execution 
Example script can be called from target console with no further restriction. For examples that support multiple backends, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./detection/example_detection_mobilenet_ssd_v2_tflite.sh
```
### C++
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
It is possible to run the object detection demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
$ ./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT_VELA} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES}
```
NOTE: For i.MX 95 use neutron converted model, a warmup time is expected.

Inference on CPU with the following script:
```bash
$ ./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2_QUANT} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
$ ./build/detection/example_detection_mobilenet_ssd_v2_tflite -p  ${MOBILENETV2} -l ${COCO_LABELS} -x ${MOBILENETV2_BOXES} -b GPU -n centeredReduced
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
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
