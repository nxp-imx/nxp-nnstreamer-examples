# Segmentation

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_segmentation_deeplab_v3_tflite.cpp](./cpp/example_segmentation_deeplab_v3_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93| deeplab_v3_mnv2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | multifilesrc<br>gst-launch<br>
[example_segmentation_deeplab_v3_tflite.sh](./example_segmentation_deeplab_v3_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93| deeplab_v3_mnv2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | multifilesrc<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93.

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./segmentation/example_segmentation_deeplab_v3_tflite.sh
```
### C++
C++ example script can be run after [cross compilation](../). To use NPU backend, use the following command, otherwise, look at the notes :
```bash
$ export MODEL_PATH="/path/to/nxp-nnstreamer-examples/downloads/models/segmentation"
$ export MODEL_MEDIA="/path/to/nxp-nnstreamer-examples/downloads/media/pascal_voc_2012_images"
$ ./build/segmentation/example_segmentation_deeplab_v3_tflite -p ${MODEL_PATH}/deeplabv3_mnv2_dm05_pascal_quant_uint8_float32.tflite -f ${MODEL_MEDIA}/image%04d.jpg
```
NOTES:
* For i.MX 93 use vela model.
* For CPU backend, use deeplabv3_mnv2_dm05_pascal.tflite model and add : -b CPU -n centeredReduced.
* For GPU backend, use deeplabv3_mnv2_dm05_pascal.tflite model and add : -b GPU -n centeredReduced.

The following execution parameters are available (Run ``` ./example_segmentation_deeplab_v3_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-p, --model_path | Use the selected model path
-f, --images_file | Use the selected images file
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.