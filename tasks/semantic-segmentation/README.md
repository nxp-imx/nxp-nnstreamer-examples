# Semantic Segmentation

## Overview
Name | Implementation | Model | ML engine | Features
--- | --- | --- | --- | ---
[example_segmentation_deeplab_v3_tflite.cpp](./cpp/example_segmentation_deeplab_v3_tflite.cpp) | C++ | DeepLabV3 | TFLite | multifilesrc<br>gst-launch<br>
[example_segmentation_deeplab_v3_tflite.sh](./example_segmentation_deeplab_v3_tflite.sh) | Bash | DeepLabV3 | TFLite | multifilesrc<br>gst-launch<br>

## DeepLabV3 segmentation
### Bash
|   Platforms  | NPU | CPU | GPU |
| ------------ | --- | --- | --- |
| i.MX 8M Plus | :white_check_mark: | :white_check_mark: | :white_check_mark: |
|   i.MX 93    | :white_check_mark: | :white_check_mark: | :x: |
|   i.MX 95    | :x: | :white_check_mark: | :x: |

The semantic segmentation demo in bash supports multiple backend for model inferences (refers to above table), default value can be overridden by explicitly defining BACKEND variable, for instance:
```bash
BACKEND=CPU ./tasks/semantic-segmentation/example_segmentation_deeplab_v3_tflite.sh
```

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

It is possible to run the semantic segmentation demo inference on three different hardwares:<br>
Inference on NPU with the following script:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES}
```
For i.MX 93 NPU use vela converted model:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT_VELA} -f ${PASCAL_IMAGES}
```
Inference on CPU with the following script:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3} -f ${PASCAL_IMAGES} -b GPU -n centeredScaled
```
The following execution parameters are available (Run ``` ./example_segmentation_deeplab_v3_tflite -h``` to see option details):

Option | Description
--- | ---
-b, --backend | Use the selected backend (CPU, GPU, NPU)<br> default: NPU
-n, --normalization | Use the selected normalization (none, centered, scaled, centeredScaled)<br> default: none
-p, --model_path | Use the selected model path
-f, --images_file | Use the selected images file
-d, --display_perf |Display performances, can specify time or freq
-t, --text_color | Color of performances displayed, can choose between red, green, blue, and black<br> default: white
-g, --graph_path | Path to store the result of the OpenVX graph compilation (only for i.MX8MPlus)<br> default: home directory

Press ```Esc or ctrl+C``` to stop the execution of the pipeline.