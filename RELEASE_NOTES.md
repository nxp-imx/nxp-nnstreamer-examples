# Release Notes

## [Release v1.5](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.5)

### Updates

| Name                  | Platforms   | Models      | Language      | Backend |
|-----------------------|-------------|-------------|---------------|---------|
| face detection        | **i.MX 95** | UltraFace   | Python/C++    | **NPU** |
| object detection      | **i.MX 95** | Yolov4-tiny | Bash + Python | **CPU** |
| sementic segmentation | **i.MX 95** | DeepLab v3  | Bash          | **CPU** |
| pose estimation       | **i.MX 95** | MoveNet     | Python        | **CPU** |
| all existing examples | **i.MX 95** |             | C++           | **CPU** |

### Other Changes
- Update download procedure to compile models with latest eIQ Toolkit version for i.MX 95 Neutron NPU 
- Add video saving for i.MX 95
- C++ examples: options for camera resolution and framerate, documentations and pipelines updates


## [Release v1.4.1](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.4.1)

### New Features

| Name                           | Platforms                     | Models  | Language | Backend         |
|--------------------------------|-------------------------------|---------|----------|-----------------|
| **all existing examples**      | i.MX 8M Plus <br> i.MX 93     |         | **C++**  | CPU / GPU / NPU |
| **monocular depth estimation** | i.MX 8M Plus <br> i.MX 93     | MidasV2 | C++      | CPU / NPU       |
| **mixed examples**             | i.MX 8M Plus <br> i.MX 93     |         | C++      | CPU / GPU / NPU |

### Updates

| Name                  | Platforms   | Models               | Language | Backend         |
|-----------------------|-------------|----------------------|----------|-----------------|
| sementic segmentation | **i.MX 93** | DeepLab v3           | Bash     | CPU / NPU       |
| pose estimation       | **i.MX 93** | MoveNet              | Python   | CPU / NPU       |
| image classification  | **i.MX 95** | MobileNet v1         | Bash/C++ | CPU / GPU / NPU |
| object detection      | **i.MX 95** | ssdlite MobileNet v2 | Bash/C++ | CPU / GPU / NPU |

### Other Changes
- Accelerated flip for Python examples
- Update and reformat pose estimation pipeline
- Fix Yolov4-tiny pre-processing

## [Release v1.3](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.3)

### New Features

| Name                 | Platforms                 | Models       | Language | Backend         |
|----------------------|---------------------------|--------------|----------|-----------------|
| image classification | i.MX 8M Plus <br> i.MX 93 | MobileNet v1 | **C++**  | CPU / GPU / NPU |

### Updates

| Name            | Platforms    | Models  | Language | Backend |
|-----------------|--------------|---------|----------|---------|
| pose estimation | i.MX 8M Plus | MoveNet | Python   | **NPU** |

### Other Changes
- DeepViewRT models removed
- Allow background execution for all examples

## [Release v1.2](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.2)

### New Features

| Name                       | Platforms                 | Models                       | Language | Backend |
|----------------------------|---------------------------|------------------------------|----------|---------|
| **emotion classification** | i.MX 8M Plus <br> i.MX 93 | UltraFace + Deepface-emotion | Python   | NPU     |

### Bug Fix
- Fix Yolov4-tiny bounding box coordinates

### Other Changes
- Store on disk result of the OpenVX graph compilation for iMX8MPlus to get the warmup time only once
- Interrupt all Python examples with `esc` or `ctrl + C`

## [Release v1.1](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.1)

### New Features

| Name                 | Platforms                 | Models                 | Language      | Backend   |
|----------------------|---------------------------|------------------------|---------------|-----------|
| object detection     | i.MX 8M Plus <br> i.MX 93 | **Yolov4 tiny**        | Bash + Python | CPU / NPU |
| **face detection**   | i.MX 8M Plus <br> i.MX 93 | UltraFace              | Python        | NPU       |
| **face recognition** | i.MX 8M Plus <br> i.MX 93 | UltraFace + FaceNet512 | Python        | NPU       |

## [Release v1.0](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.0)

### New Features

| Name                      | Platforms                 | Models               | Language | Backend         |
|---------------------------|---------------------------|----------------------|----------|-----------------|
| **image classification**  | i.MX 8M Plus <br> i.MX 93 | MobileNet v1         | Bash     | CPU / GPU / NPU |
| **object detection**      | i.MX 8M Plus <br> i.MX 93 | ssdlite MobileNet v2 | Bash     | CPU / GPU / NPU |
| **sementic segmentation** | i.MX 8M Plus              | DeepLab v3           | Bash     | CPU / GPU / NPU |
| **pose estimation**       | i.MX 8M Plus              | MoveNet              | Python   | CPU             |
