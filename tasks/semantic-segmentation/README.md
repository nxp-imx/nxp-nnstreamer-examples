# Semantic Segmentation

## Overview
Name | Implementation | Model | ML engine | Features
--- | --- | --- | --- | ---
[example_segmentation_deeplab_v3_tflite.cpp](./cpp/example_segmentation_deeplab_v3_tflite.cpp) | C++ | DeepLabV3 | TFLite | jpeg files slideshow<br>gst-launch<br>
[example_segmentation_deeplab_v3_tflite.sh](./example_segmentation_deeplab_v3_tflite.sh) | Bash | DeepLabV3 | TFLite | jpeg files slideshow<br>gst-launch<br>

## DeepLabV3 segmentation
### Bash Execution

The semantic segmentation demo in bash supports multiple backend for model inferences, default value can be overridden by explicitly defining BACKEND variable, for instance:
```bash
BACKEND=NPU ./tasks/semantic-segmentation/example_segmentation_deeplab_v3_tflite.sh
```

### C++ Execution

C++ example script needs to be generated with [cross compilation](../). [setup_environment.sh](../tools/setup_environment.sh) script needs to be executed in [nxp-nnstreamer-examples](../) folder to define data paths:
```bash
. ./tools/setup_environment.sh
```

#### NPU Inference

For i.MX 8M Plus (VSI NPU):
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES}
```

For i.MX 93 (Ethos-U65):
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT_VELA} -f ${PASCAL_IMAGES}
```

#### Inferences on other hardwares

Inference on CPU with the following script:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3_QUANT} -f ${PASCAL_IMAGES} -b CPU
```
Quantized model is used for better inference performances on CPU.<br>
NOTE: inferences on i.MX8MPlus GPU have low performances, but are possible with the following script:
```bash
./build/semantic-segmentation/example_segmentation_deeplab_v3_tflite -p ${DEEPLABV3} -f ${PASCAL_IMAGES} -b GPU -n centeredScaled
```

#### C++ Execution Parameters

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