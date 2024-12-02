# Segmentation

## Overview
Name | Implementation | Platforms | Model | ML engine | Backend | Features
--- | --- | --- | --- | --- | --- | ---
[example_segmentation_deeplab_v3_tflite.cpp](./cpp/example_segmentation_deeplab_v3_tflite.cpp) | C++ | i.MX 8M Plus <br> i.MX 93| deeplab_v3_mnv2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | multifilesrc<br>gst-launch<br>
[example_segmentation_deeplab_v3_tflite.sh](./example_segmentation_deeplab_v3_tflite.sh) | Bash | i.MX 8M Plus <br> i.MX 93| deeplab_v3_mnv2 | TFLite | NPU (defaut)<br>GPU<br>CPU<br> | multifilesrc<br>gst-launch<br>

NOTES:
* No GPU support on i.MX 93

## Execution
Example script can be called from target console with no further restriction. For examples that support multiple backend, default value can be overriden by explicitly defining BACKEND variable, for instance:
### Bash
```bash
$ BACKEND=CPU ./segmentation/example_segmentation_deeplab_v3_tflite.sh
```
### C++
C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
$ . ./tools/setup_environment.sh
```
It is possible to run the segmentation demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
$ ./build/segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES}
```
For i.MX 93 use vela converted model:
```bash
$ ./build/segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT_VELA} -f ${PASCAL_IMAGES}
```
Inference on CPU with the following script:
```bash
$ ./build/segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
$ ./build/segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3} -f ${PASCAL_IMAGES} -b GPU -n centeredReduced
```
The following execution parameters are available (Run ``` ./example_segmentation_deeplab_v3_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, reduced, centeredReduced, castInt32, castuInt8)<br> default: none
-p, --model_path | Use the selected model path
-f, --images_file | Use the selected images file
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black (white by default)
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.