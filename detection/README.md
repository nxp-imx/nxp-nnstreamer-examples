# Object detection

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_detection_mobilenet_ssd_v2_tflite.cpp](./cpp/example_detection_mobilenet_ssd_v2_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_mobilenet_ssd_v2_tflite.sh](./example_detection_mobilenet_ssd_v2_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | TFLite | NPU (default)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_yolo_v4_tiny_tflite.sh](./example_detection_yolo_v4_tiny_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93 | yolov4_tiny | TFLite | NPU (default)<br>CPU<br> | camera<br>gst-launch<br>[custom python tensor_filter](./postprocess_yolov4_tiny.py)

NOTES:
* No GPU support on i.MX 93.
* Yolov4-tiny output does not directly work with the Yolov5 mode of tensor_decoder element, so a python filter is used to post-process and reshape this output as required.
 
## Execution 
Example script can be called from target console with no further restriction. For examples that support multiple backends, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./detection/example_detection_mobilenet_ssd_v2_tflite.sh
```
### C++
C++ example script can be run after [cross compilation](../). To use NPU backend, use the following command, otherwise, look at the notes :
```bash
$ export MODEL_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/detection"
$ ./build/detection/example_detection_mobilenet_ssd_v2_tflite -p ${MODEL_PATH}/ssdlite_mobilenet_v2_coco_quant_uint8_float32_no_postprocess.tflite -l ${MODEL_PATH}/coco_labels_list.txt -x ${MODEL_PATH}/box_priors.txt
```
NOTES:
* For i.MX 93 use vela model.
* For CPU backend, use ssdlite_mobilenet_v2_coco_no_postprocess.tflite model and add : -b CPU -n centeredReduced.
* For GPU backend, use ssdlite_mobilenet_v2_coco_no_postprocess.tflite model and add : -b GPU -n centeredReduced.

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

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.
