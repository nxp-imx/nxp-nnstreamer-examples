# NXP NNStreamer examples
Purpose of this repository is to provide, for demonstration purpose, functional examples of GStreamer/NNStreamer-based pipelines optimized and validated for some designated NXP i.MX application processors.

# How to run examples
## Models and metadata download
Models and metadata files used by examples are not archived in this repository.
Therefore they have to be downloaded over the network, prior to execution of the examples on the target. Download of those files is to be done from the host PC, running Jupyter Notebook [download.ipynb](./downloads).
Refer to [download instruction](./downloads/README.md) for details.
## Execution on target
### Python
Once models have been fetched locally on host PC, repository will contain both examples and downloaded artifacts. Thus it can be uploaded to the target board for individual examples execution. Complete repository can either be uploaded from host PC to target using regular `scp` command or only the necessary directories using [upload.sh](./tools/upload.sh) script provided for host:
```bash
# replace <target ip address> by relevant value
$ cd /path/to/nxp-nnstreamer-examples
$ ./tools/upload.sh root@<target ip address>
```
### C++ with cross-compilation
1- Fetch the models locally on host PC
2- Download or build a Yocto BSP SDK for the i.MX platform where example will be run on
3- Open a terminal and navigate to the sources of the nxp-nnstreamer-example git tree
4- SDK environment setup script must be executed before the SDK can be used
5- Once the SDK is setup, the right path to the models must be specified in example files
6- Build the project with CMake, then push the required artifacts on the board (the scp command can be used for this purpose)

Example :
```bash
# Source the SDK
$ .  /path/to/sdk/environment-setup-armv8-poky-linux
# Cross-compile examples
$ cd /path/to/nxp-nnstreamer-examples
$ mkdir build
$ cd build
$ cmake ..
$ make
# Send classification example to your board, replace <target ip address> by relevant value
$ scp ./classification/example_classification_mobilenet_v1_tflite root@<target ip address>
```

Examples can then be run directly on the target. More information on individual examples execution is available in relevant sections.

# Categories
Note that examples may not run on all platforms - check table below for platform compatibility.

Snapshot | Name | Platforms | Features
--- | --- | --- | ---
[![object detection demo](./detection/detection_demo.webp)](./detection/) | [Object detection](./detection/) | i.MX 8M Plus <br> i.MX 93| MobileNet SSD <br> Yolov4-tiny <br> TFLite / DeepViewRT <br> v4l2 camera <br> gst-launch <br> [custom python tensor_filter](./detection/postprocess_yolov4_tiny.py)
[![image classification demo](./classification/classification_demo.webp)](./classification/) | [Image classification](./classification/) | i.MX 8M Plus <br> i.MX 93| MobileNet <br> TFLite / DeepViewRT <br> v4l2 camera <br> gst-launch
[![image segmentation demo](./segmentation/segmentation_demo.webp)](./segmentation/) | [Image segmentation](./segmentation/) | i.MX 8M Plus | DeepLab<br> TFLite<br> jpeg files slideshow<br> gst-launch
[![pose detection demo](./pose/pose_demo.webp)](./pose/) | [Pose detection](./pose/) |i.MX 8M Plus | MoveNet<br> TFLite <br> video file decoding <br> python
[![faces demo](./face/face_demo.webp)](./face/) | [Face](./face/) | i.MX 8M Plus <br> i.MX 93| UltraFace <br> FaceNet512 <br> Deepface-emotion <br> TFLite <br> v4l2 camera <br> python

*Images and video used have been released under Creative Commons CC0 1.0 license or belong to Public Domain*