# Copyright 2023 NXP
# SPDX-License-Identifier: BSD-3-Clause

import numpy as np
import nnstreamer_python as nns


def post_process_yolo(features, output_tensor, offset, anchors, factor, threshold,
                      cell_size, stride, image_dim, num_anchors):
    """Apply sigmoid on useful values from the Yolov4-tiny output tensors

    Applying sigmoid on all values is CPU demanding and makes the filter slow
    To make the filter much faster, sigmoid is only applied on values over the threshold that could lead to a prediction
    Then, sigmoid results are stored in the initialized output tensor
    values where sigmoid is not applied are left at zero

    Arguments :
    features      - the Yolov4-tiny output tensor to process
    output_tensor - a numpy zero tensor that will be filled with processed values
    offset        - offset to write values in output_tensor at the right position
    anchors       - list of anchor boxes used by Yolov4-tiny
    factor        - sigmoid factor
    threshold     - detection threshold, all values under this threshold will not be kept
    cell_size     - size of a cell in pixels
    stride        - stride in pixels between two cells
    image_dim     - model input image size
    num_anchors   - number of anchor boxes used by Yolov4-tiny
    """

    # the fifth coordinate of a box corresponds to the confidence that this cell contains an object
    cell_confidence = features[..., 4]
    over_threshold_list = np.where(cell_confidence > threshold)

    # sigmoid is only applied where confidence is over threshold
    # thus, processing is faster than applying sigmoid to all values
    if over_threshold_list[0].size > 0:
        indices = np.array(over_threshold_list[0])
        indices_offset = indices + offset

        box_positions = np.floor_divide(indices, num_anchors)

        list_xy = np.array(np.divmod(box_positions, cell_size)).T
        list_yx = list_xy[..., ::-1]
        boxes_yx = np.reshape(list_yx, (int(list_yx.size / 2), 2))

        # boxes center coordinates
        output_tensor[indices_offset, :2] = \
            ((np_sigmoid(features[indices, :2] * factor) - 0.5 * (factor - 1) + boxes_yx) * stride) / image_dim

        # boxes width and height
        output_tensor[indices_offset, 2:4] = \
            (np.exp(features[indices, 2:4]) * anchors[np.divmod(indices, num_anchors)[1]]) / image_dim

        # confidence that cell contains an object
        output_tensor[indices_offset, 4:5] = np_sigmoid(features[indices, 4:5])

        # class with the highest probability in this cell
        max_class_indices = np.argmax(features[indices, 5:], axis=1) + 5
        output_tensor[indices_offset, max_class_indices] = np_sigmoid(np.amax(features[indices, max_class_indices]))

    return output_tensor


def np_sigmoid(x): return 1 / (1 + np.exp(-x))


def reciprocal_sigmoid(x): return -np.log(1 / x - 1)


class CustomFilter(object):
    """This filter is made to postprocess and reshape the output of Yolov4-tiny

    The output of tensor_filter inference with Yolov4-tiny model contains two tensors
    Their dimensions are respectively (1, 13, 13, 255) and (1, 26, 26, 255)
    The two dimensions (13,13) or (26,26) correspond to the position of a cell along x and y axes
    Input images are 416*416 pixels and Yolov4-tiny use 32 and 16 strides
    (416 * 416) / 32 = 13 * 13 boxes for the 32 stride
    (416 * 416) / 16 = 26 * 26 boxes for the 16 stride
    The last dimension (255) corresponds to cell data :
    each box has 3 anchor boxes, and for each anchor box there are 5 cell parameters and 80 classes (model trained on COCO dataset)
    so there are 3 * (5 + 80) = 255 data for each box
    Cell parameters used:
    - parameters 0 and 1 correspond to x and y coordinates of cell center
    - parameters 2 and 3 correspond to width and height of the cell
    - parameter 4 corresponds to the object score, which is the confidence that this cell contains an object

    This filter must postprocess and merge Yolov4-tiny output tensors

    tensor_decoder with Yolov5 decoder mode requires one single input tensor with specific dimensions
    The decoder expects to have cells with 32, 16 and 8 strides, and 3 anchor boxes
    Regular Yolo models have 8-stride cells, but tiny models don't to be lighter and faster
    (416 * 416) / 8 = 52 * 52 boxes are expected for the 8 stride
    Each cell must contain 85 data which are the 5 cell parameters and the 80 classes
    The expected dimensions are (1, 3 * (13 ** 2 + 26 ** 2 + 52 ** 2), 85) = (1, 10647, 85)
    so this filter must output a single output tensor with (1, 10647, 85) dimensions
    The two Yolov4-tiny output tensors must then be merged in this output tensor

    tensor_decoder post-processing requires sigmoid is applied on inference results
    Such processing is made by the Yolov5 model used as a reference for NNStreamer Yolov5 decoder mode
    This Yolov4-tiny model do not apply sigmoid on output tensors, so it must be done by this filter
    """

    image_dims = []
    output_dims = []
    stride_size = [32, 16]  # There is no 8-stride cells for tiny version of Yolov4 model
    cell_size = []
    data_size = 0
    fake_stride = 8  # A dummy 8-stride tensor will be added to match expected input dimensions of tensor_decoder
    fake_data_size = 0

    threshold = reciprocal_sigmoid(0.25)  # default threshold in NNStreamer tensor_decoder element is 0.25
    num_anchor_boxes = 3
    num_classes = 80
    detection_num_info = 5
    total_classes_data = num_classes + detection_num_info

    def __init__(self, *args):

        custom_input = args[0].split(',')
        custom_height = custom_input[0].split(':')
        custom_width = custom_input[1].split(':')
        assert custom_height[0] == 'Height', "first custom argument Height is required, should be Height:N"
        assert custom_width[0] == 'Width', "second custom argument Width is required, should be Width:N"
        dims = list(map(int, [custom_height[1], custom_width[1]]))

        if 'Threshold:' in args[0]:
            input_threshold = float(args[0].split('Threshold:')[1])
            assert 0. <= input_threshold <= 1., "threshold must be between 0 and 1"
            self.threshold = reciprocal_sigmoid(input_threshold)

        assert len(dims) == 2, "Image dimension should have this format: N:N (e.g., 416:416)"
        assert dims[0] == dims[1], "input image should be square"

        self.image_dims = dims
        self.cell_size = [int(dims[0] / self.stride_size[0]), int(dims[1] / self.stride_size[1])]
        self.data_size = self.cell_size[0] ** 2 + self.cell_size[1] ** 2

        fake_cell_size = int(dims[0] / self.fake_stride)
        self.fake_data_size = self.cell_size[0] ** 2 + self.cell_size[1] ** 2 + fake_cell_size ** 2

    def setInputDim(self, input_dims):

        assert (len(input_dims) == 2), "Must be 2 output tensors for a Yolov4-tiny model"
        num_classes_and_anchors = self.total_classes_data * self.num_anchor_boxes
        dim_input_1 = input_dims[0].getDims()
        dim_input_2 = input_dims[1].getDims()
        assert (dim_input_1[0] == dim_input_2[0] == num_classes_and_anchors), "Wrong number of classes or anchors"
        input_num_data = dim_input_1[1] * dim_input_1[2] + dim_input_2[1] * dim_input_2[2]
        assert (input_num_data == self.data_size), "Wrong sizes of tensors"
        self.output_type = input_dims[0].getType()
        shape_output_tensor = [self.total_classes_data, self.num_anchor_boxes * self.fake_data_size, 1]

        self.output_dims = [nns.TensorShape(shape_output_tensor, self.output_type)]

        return self.output_dims

    def invoke(self, input_array):
        """ Post-processing : decode & reshape Yolov4_tiny output tensors
        return a tensor with tensor_decoder required shape and content

        argument :
        input_array : output tensors from Yolov4-tiny, computed at the previous tensor_filter

        output :
        output_tensor : tensor in the expected dimension for tensor_decoder
                        contains post-processed Yolov4-tiny output tensors

        the filter output tensor is created with a (1, 10647, 85) dimension and is initialized with zeros
        This way, this tensor will contain the missing 8-stride cells to match output dimension requirements
        The threshold (> 0) ensures that these values initialized with zeros will be dropped during tensor_decoder postprocessing

        Postprocessing of tensor_decoder element using Yolov5 decoder mode consists in:
        - determining which boxes contain a class
        - decoding boxes to obtain their coordinates and sizes
        - performing nms to remove overlapping boxes

        This postprocessing requires that sigmoid function is applied on Yolov4-tiny output tensors
        This implementation of Yolov4-tiny does not apply sigmoid on output tensors, it must be done by this filter
        """

        total_classes = self.total_classes_data
        num_anchors = self.num_anchor_boxes
        image_dim = self.image_dims[0]
        threshold = self.threshold
        offsets = [0, num_anchors * self.cell_size[0] ** 2]

        # Anchors, its values and sigmoid factor were pulled out of config file from original Darknet repository
        anchors = [[[81, 82], [135, 169], [344, 319]], [[23, 27], [37, 58], [81, 82]]]
        sigmoid_factor = [1.05, 1.05]

        input_tensor_1 = np.reshape(input_array[0], (num_anchors * self.cell_size[0] ** 2, total_classes))
        input_tensor_2 = np.reshape(input_array[1], (num_anchors * self.cell_size[1] ** 2, total_classes))
        input_tensors = [input_tensor_1, input_tensor_2]

        output_tensor = np.zeros((num_anchors * self.fake_data_size, total_classes), dtype=self.output_type)

        for idx, tensor in enumerate(input_tensors):
            offset = offsets[idx]  # offset to place processed values at the right place in the output tensor
            anchor = np.array(anchors[idx])
            factor = sigmoid_factor[idx]
            cell_size = self.cell_size[idx]
            stride_size = self.stride_size[idx]

            output_tensor = post_process_yolo(tensor, output_tensor, offset, anchor,
                                              factor, threshold, cell_size, stride_size,
                                              image_dim, num_anchors)

        return [np.ravel(output_tensor)]
