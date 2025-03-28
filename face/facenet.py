#!/usr/bin/env python3
#
# Copyright 2022-2023, 2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import numpy as np
import os


class FNModel:

    def __init__(self, model_directory, database_directory,
                 match_threshold=1.0, vela=False):
        """Helper class for FaceNet model.

        model_directory: directory where tflite model is located
        database_directory: directory for face / embedding bundle records
        match_threshold: threshold for euclidean distance match comparison
            (the lower the stricter)
        vela: use vela version of the model
        """
        # Face detection definitions
        self.MODEL_FACENET_WIDTH = 160
        self.MODEL_FACENET_HEIGHT = 160
        self.MODEL_FACENET_EMBEDDING_LEN = 512

        self.match_threshold = match_threshold

        # model location
        if vela:
            name = 'facenet512_uint8_vela.tflite'
        else:
            name = 'facenet512_uint8.tflite'
        self.tflite_model = os.path.join(model_directory, name)

        if not os.path.exists(self.tflite_model):
            raise FileExistsError(f'cannot find model [{self.tflite_model}]')

        if database_directory is not None:
            if not os.path.exists(database_directory):
                os.mkdir(database_directory)
        self.database_dir = database_directory

    def get_model_path(self):
        """Get full path to model file.
        """
        return self.tflite_model

    def get_model_input_shape(self):
        """Get dimensions of model input tensor.
        """
        return (self.MODEL_FACENET_HEIGHT, self.MODEL_FACENET_WIDTH, 3)

    def get_model_output_shape(self):
        """Get dimensions of model output tensors (list).
        """
        return [(1, self.MODEL_FACENET_EMBEDDING_LEN)]

    def db_search_match(self, database, embedding):
        """Compare embedding with database to find a match.

        Criteria is based on euclidean distance of normalized vectors.

        database: memory database with name / vectors bundle
        embedding: reference embedding to be matched
        returns: tuple (name, confidence [0, 1.0])
        """
        best = (None, self.match_threshold)
        if not self.db_check_embedding_format(embedding):
            raise ValueError(f'embedding numpy format error:\n {embedding}')
        if database is None:
            return best
        normalized = embedding / np.linalg.norm(embedding)
        for _name, _embedding in database.items():
            # _embedding is already L2-normalized
            distance = np.linalg.norm(normalized - _embedding)
            if (distance < best[1]):
                best = (_name, distance)
        return best

    def db_check_embedding_format(self, embedding):
        """Check consistency of embedding array with the model.

        embedding: vector to be checked
        """
        shape = embedding.shape
        type = embedding.dtype

        if len(shape) == 1 and \
                shape[0] == self.MODEL_FACENET_EMBEDDING_LEN and \
                type == np.float32:
            return True
        else:
            return False

    def db_save_record(self, name, embedding):
        """Create a file to store name and embedding.

        name: name associated to the new record
        embedding: raw embedding (not normalized) associated to this name
        """
        if not self.db_check_embedding_format(embedding):
            raise ValueError(f'embedding numpy format error [{embedding}]')

        file = os.path.join(self.database_dir, name)
        np.save(file, embedding)

    def db_add_record(self, database, name, embedding):
        """Add entry in memory database for an embedding associated to a name.

        database: memory data base for face / embedding bundles
        name: name associated to the new record
        embedding: raw embedding (not normalized) array associated to this name
        """
        if database is None:
            database = {}
        if not self.db_check_embedding_format(embedding):
            raise ValueError(f'embedding numpy format error [{embedding}]')

        normalized = embedding / np.linalg.norm(embedding)
        database[name] = normalized

    def db_load(self):
        """Populate database in memory from face embeddings stored in files.

        File name <name>.npy contain raw numpy array embedding for <name>
        Embeddings are stored into files as raw without normalization.
        """
        database = {}
        for file in os.listdir(self.database_dir):
            if file.endswith(".npy"):
                name = os.path.splitext(file)[0]
                npfile = os.path.join(self.database_dir, file)
                array = np.load(npfile)
                self.db_add_record(database, name, array)
        return database
