# Purpose of [postprocess_yolov4_tiny.py](./postprocess_yolov4_tiny.py):

- This Python filter is made to postprocess and reshape the output of Yolov4-tiny, to match input format from NNStreamer tensor_decoder element using yolov5 decoder mode.
- To call this Python filter, a tensor_filter element must be used in the NNStreamer pipeline flow.
Path of the Python filter must be given as the model argument, and the framework argument is python3.
- It operates just after the inference of Yolov4-tiny, and before the tensor_decoder element.

## Custom option available: detection threshold value

Detection threshold value can be easily changed in [example_detection_yolo_v4_tiny_tflite.sh](./example_detection_yolo_v4_tiny_tflite.sh).
Threshold parameter is optional, and should be defined between 0 and 1 (default = 0.25).

If this threshold is close to 1, fewer objects may be detected but predictions will be more reliable.
At the opposite with a threshold close to 0, the model should detect more objects but less reliably.

tensor_decoder threshold value is hard-coded at 0.25 in [tensordec-boundingbox.c](https://github.com/nnstreamer/nnstreamer/blob/2e6a6989a5befeff289e4a7825dc9ff03aa5467b/ext/nnstreamer/tensor_decoder/tensordec-boundingbox.c#L131) in the NNStreamer repository.
To set the threshold value under 0.25, hard-coded value in tensordec-boundingbox.c must also be reduced.