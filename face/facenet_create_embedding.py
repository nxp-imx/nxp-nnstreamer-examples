#!/usr/bin/env python3
#
# Copyright 2022-2023, 2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import numpy as np
import os
import tensorflow as tf
from PIL import Image


def load_image(image_path, crop_box=None, resize_wh=None):
    image = Image.open(image_path)
    image = image.convert(mode='RGB')
    if crop_box is not None:
        left = crop_box[0]
        upper = crop_box[1]
        right = crop_box[0] + crop_box[2]
        lower = crop_box[1] + crop_box[3]
        image = image.crop((left, upper, right, lower))
    if resize_wh is not None:
        image = image.resize(resize_wh)
    image.show()
    array = np.array(image)
    array = np.expand_dims(array, axis=0)
    return array


def inference(model_path, image_path, crop_box=None, resize_wh=None):
    # crop_box (x0,y0, w, h) tuple
    # resize_wh (w,y) tuple
    interpreter = tf.lite.Interpreter(model_path=model_path)
    interpreter.allocate_tensors()
    output = interpreter.get_output_details()[0]
    input = interpreter.get_input_details()[0]
    print(input)
    input_data = load_image(image_path, crop_box=crop_box, resize_wh=resize_wh)
    interpreter.set_tensor(input['index'], input_data)
    interpreter.invoke()
    out0 = interpreter.get_tensor(output['index'])
    return out0


if __name__ == "__main__":
    pwd = os.path.dirname(os.path.abspath(__file__))

    model_path = os.path.join(
        pwd, '../downloads/models/face/facenet512_uint8.tflite')
    model_wh = (160, 160)

    # Default crop parameters correspond to thispersondoesnotexist.jpeg
    img_path_default = os.path.join(pwd, 'thispersondoesnotexist.jpeg')
    x0_default = 227
    y0_default = 317
    width_height_default = 610

    parser = argparse.ArgumentParser(description='Face Identification')
    parser.add_argument('--img_path', type=str,
                        help='image path', default=img_path_default)
    parser.add_argument('--x0', type=int,
                        help='cropped area x0 coordinate', default=x0_default)
    parser.add_argument('--y0', type=int,
                        help='cropped area y0 coordinate', default=y0_default)
    parser.add_argument('--width_height', type=int,
                        help='cropped area width/height',
                        default=width_height_default)

    args = parser.parse_args()
    img_path = args.img_path
    x0 = args.x0
    y0 = args.y0
    width_height = args.width_height

    img_box = (x0, y0, width_height, width_height)

    print(f'# Compute embedding for {img_path} ')
    print(f'# Cropped area (x0, y0)=({x0}, {y0}) width/height={width_height}')

    embed = inference(
        model_path,
        img_path,
        crop_box=img_box,
        resize_wh=model_wh)
    embed0 = embed[0]

    # store resulting embedding vector as numpy file
    npy_file = os.path.splitext(os.path.basename(img_path))[0] + '.npy'
    embedding = os.path.join(pwd, npy_file)

    print(f'# Embedding file saved to {embedding}')
    np.save(embedding, embed0)
