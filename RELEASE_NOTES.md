# Release Notes

## [Release v1.4.1](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.4.1)

### New Features

| Name           | Platforms                     | Models     | Language | Backend         |
|----------------|-------------------------------|------------|----------|-----------------|
| all demos      | i.MX 8M Plus <br> i.MX 93     |            | **C++**  | CPU / GPU / NPU |
| **depth**      | i.MX 8M Plus <br> i.MX 93     | MidasV2    |  **C++** | **CPU / NPU**   |
| **mixed examples** | i.MX 8M Plus <br> i.MX 93 |            | **C++**  | CPU / GPU / NPU |
| segmentation   | i.MX 8M Plus <br> **i.MX 93** | DeepLab v3 | Bash     | CPU / GPU / NPU |
| pose           | i.MX 8M Plus <br> **i.MX 93** | MoveNet    | Python   | CPU / NPU       |
| classification | i.MX 8M Plus <br> i.MX 93 <br> **i.MX 95** | MobileNet v1         | Bash/C++     | CPU / GPU / NPU |
| detection      | i.MX 8M Plus <br> i.MX 93 <br> **i.MX 95** | ssdlite MobileNet v2 | Bash/C++     | CPU / GPU / NPU |

### Other Changes
- Mixed examples 
- Accelerated flip for Python examples
- Update and reformat pose pipeline
- Fix Yolov4-tiny pre-processing

## [Release v1.3](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.3)

### New Features

| Name           | Platforms                 | Models       | Language | Backend         |
|----------------|---------------------------|--------------|----------|-----------------|
| classification | i.MX 8M Plus <br> i.MX 93 | MobileNet v1 | **C++**  | CPU / GPU / NPU |
| pose           | i.MX 8M Plus              | MoveNet      | Python   | CPU / **NPU**   |

### Other Changes
- DeepViewRT models removed
- Allow background execution for all examples

## [Release v1.2](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.2)

### New Features

| Name                  | Platforms                 | Models                       | Language | Backend |
|-----------------------|---------------------------|------------------------------|----------|---------|
| **emotion detection** | i.MX 8M Plus <br> i.MX 93 | UltraFace + Deepface-emotion | Python   | NPU     |

### Bug Fix
- Fix Yolov4-tiny bounding box coordinates

### Other Changes
- Store on disk result of the OpenVX graph compilation for iMX8MPlus to get the warmup time only once
- Interrupt all Python examples with `esc` or `ctrl + C`

## [Release v1.1](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.1)

### New Features

| Name                                        | Platforms                 | Models                                | Language    | Backend   |
|---------------------------------------------|---------------------------|---------------------------------------|-------------|-----------|
| detection                                   | i.MX 8M Plus <br> i.MX 93 | **Yolov4 tiny**                       | Bash/Python | CPU / NPU |
| **face detetion** <br> **face recognition** | i.MX 8M Plus <br> i.MX 93 | UltraFace <br> UltraFace + FaceNet512 | Python      | NPU       |

## [Release v1.0](https://github.com/nxp-imx/nxp-nnstreamer-examples/tree/v1.0)

### New Features

| Name               | Platforms              | Models               | Language | Backend         |
|--------------------|------------------------|----------------------|----------|-----------------|
| **classification** | i.MX 8M Plus <br> i.MX 93 | MobileNet v1         | Bash     | CPU / GPU / NPU |
| **detection**      | i.MX 8M Plus <br> i.MX 93 | ssdlite MobileNet v2 | Bash     | CPU / GPU / NPU |
| **segmentation**   | i.MX 8M Plus           | DeepLab v3           | Bash     | CPU / GPU / NPU |
| **pose**           | i.MX 8M Plus           | MoveNet              | Python   | CPU             |
