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
1- Fetch the models locally on host PC. <br>
2- Build a Yocto BSP SDK for the dedicated i.MX platform. Refer to the [imx-manifest](https://github.com/nxp-imx/imx-manifest) to setup the correct building environment, SDK needs to be compiled with bitbake using `imx-image-full` and `populate_sdk` command as followed :<br>
```bash
# Build the image recipe imx-image-full
$ bitbake imx-image-full -c populate_sdk
```
3- The SDK environment setup script located in `/path/to/yocto/bld-xwayland/tmp/deploy/sdk/` must be executed before being able to source its environment. <br>
4- Source the SDK environment and compile C++ examples with CMake, then push the required artifacts in its expected folder on the board (the scp command can be used for this purpose). <br>
Note: path to the folder containing the data can be changed in CMakeLists.txt file as well as model and label names in the cpp example source file.
Example :
```bash
# Source the SDK (installed by default in /opt/fsl-imx-xwayland/<LF version>/)
$ . /path/to/sdk/environment-setup-armv8-poky-linux
# Cross-compile examples
$ cd /path/to/nxp-nnstreamer-examples
$ mkdir build && cd $_
$ cmake ..
$ make
# Send classification example to target, replacing <target ip address> by relevant value
$ scp ./classification/example_classification_mobilenet_v1_tflite root@<target ip address>
```
C++ examples were developped using a custom C++ library. A description of how to use the library can be found [here](./common/cpp/README.md).
### Compile models on target
Quantized TFLite models must be compiled with vela for i.MX 93 Ethos-U NPU.
This can be done directly on the target :
```bash
# To do directly on target
$ cd /path/to/nxp-nnstreamer-examples
$ ./downloads/compile_models.sh
```

Examples can then be run directly on the target. More information on individual examples execution is available in relevant sections.

# Categories
Note that examples may not run on all platforms - check table below for platform compatibility.

Snapshot | Name | Platforms | Features
--- | --- | --- | ---
[![object detection demo](./detection/detection_demo.webp)](./detection/) | [Object detection](./detection/) | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95| MobileNet SSD <br> Yolov4-tiny <br> TFLite <br> v4l2 camera <br> gst-launch <br> [custom python tensor_filter](./detection/postprocess_yolov4_tiny.py)
[![image classification demo](./classification/classification_demo.webp)](./classification/) | [Image classification](./classification/) | i.MX 8M Plus <br> i.MX 93 <br> i.MX 95| MobileNet <br> TFLite <br> v4l2 camera <br> gst-launch
[![image segmentation demo](./segmentation/segmentation_demo.webp)](./segmentation/) | [Image segmentation](./segmentation/) | i.MX 8M Plus <br> i.MX 93| DeepLab<br> TFLite<br> jpeg files slideshow<br> gst-launch
[![pose detection demo](./pose/pose_demo.webp)](./pose/) | [Pose detection](./pose/) |i.MX 8M Plus <br> i.MX 93| MoveNet<br> TFLite <br> video file decoding (i.MX 8M Plus only)<br> v4l2 camera <br> gst-launch <br> python
[![faces demo](./face/face_demo.webp)](./face/) | [Face](./face/) | i.MX 8M Plus <br> i.MX 93| UltraFace <br> FaceNet512 <br> Deepface-emotion <br> TFLite <br> v4l2 camera <br> python
[![mixed demo](./mixed/mixed_demo.webp)](./mixed/) | [Mixed](./mixed/) | i.MX 8M Plus <br> i.MX 93| MobileNet SSD <br> MobileNet <br> Movenet <br> UltraFace <br> Deepface-emotion <br> TFLite <br> v4l2 camera <br> C++ <br> custom C++ decoding
[![depth demo](./depth/depth_demo.webp)](./depth/) | [Depth](./depth/) | i.MX 8M Plus <br> i.MX 93| Midas v2 <br> TFLite <br> v4l2 camera <br> C++ <br> custom C++ decoding

*Images and video used have been released under Creative Commons CC0 1.0 license or belong to Public Domain*