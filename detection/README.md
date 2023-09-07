# Object detection

## Overview
Name | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | ---
[example_detection_mobilenet_ssd_v2_tflite.sh](./example_detection_mobilenet_ssd_v2_tflite.sh) | i.MX 8M Plus <br> i.MX 93 | mobilenet_ssd_v2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | camera<br>gst-launch<br>
[example_detection_yolo_v4_tiny_tflite.sh](./example_detection_yolo_v4_tiny_tflite.sh) | i.MX 8M Plus <br> i.MX 93 | yolov4_tiny | TFLite | NPU (defaut)<br>CPU<br> | camera<br>gst-launch<br>[custom python tensor_filter](./postprocess_yolov4_tiny.py)

NOTES:
* No GPU support on i.MX 93
* Yolov4-tiny output does not directly work with the Yolov5 mode of tensor_decoder element, so a python filter is used to post-process and reshape this output as required
 
## Execution 
Example script can be called from target console with no further restriction. For examples that support multiple backends, default value can be overriden by explicitly defining BACKEND variable, for instance:
```
# BACKEND=CPU ./detection/example_detection_mobilenet_ssd_v2_tflite.sh
```
