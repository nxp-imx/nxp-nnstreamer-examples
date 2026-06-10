# Tasks

This directory contains various machine learning task examples demonstrating NNStreamer capabilities on NXP platforms.

## NXP eIQ Model Zoo

For pre-trained models and conversion scripts used in these tasks, please refer to the [NXP eIQ Model Zoo](https://github.com/NXP/eiq-model-zoo).

## NPU Enablement by Platform

| Task | Model | i.MX 8M Plus (VSI NPU) | i.MX 93 (Ethos-U65) | i.MX 95 (Neutron) | i.MX 952 (Neutron) |
|------|------|------|-------|------|------|
| Classification | MobileNetV1 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Object Detection | YOLOv4 Tiny | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Object Detection | SSD MobileNetV2 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Semantic Segmentation | DeepLabV3 | :white_check_mark: | :white_check_mark: | :x: | :x: |
| Pose Estimation | MoveNet Lightning | :white_check_mark: | :x: | :x: | :x: |
| Monocular Depth Estimation | MiDaS v2 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Face Detection | UltraFace-slim | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Emotion Classification | Deepface-emotion | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Face Recognition | FaceNet512 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Classification + Object Detection | MobileNetV1 + SSD MobileNetV2 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |
| Pose Estimation + Face Detection | MoveNet Lightning + UltraFace-slim | :white_check_mark: | :white_check_mark: | :x: | :x: |
| Double Classification | 2x MobileNetV1 | :white_check_mark: | :white_check_mark: | :white_check_mark: | :white_check_mark: |

