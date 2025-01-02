#!/usr/bin/env python3
#
# Copyright 2022-2025 NXP
# SPDX-License-Identifier: BSD-3-Clause

import logging
import re
import subprocess
from enum import IntEnum, auto


class SocId(IntEnum):
    """i.MX devices enumeration.
    """
    def __str__(self):
        return str(self.name)
    IMX8MQ = auto()
    IMX8MM = auto()
    IMX8MN = auto()
    IMX8MP = auto()
    IMX8ULP = auto()
    IMX8QM = auto()
    IMX8QXP = auto()
    IMX93 = auto()
    IMX95 = auto()
    UNKNOWN = auto()


class Feature(IntEnum):
    """i.MX Features enumeration.
    """
    def __str__(self):
        return str(self.name)
    GPU2D = auto()
    GPU3D = auto()
    NPU = auto()


# machine regex used for SoC identification
mach_regex_to_soc = [('imx8mq', SocId.IMX8MQ),
                     ('imx8mm', SocId.IMX8MM),
                     ('imx8mn', SocId.IMX8MN),
                     ('imx8mp', SocId.IMX8MP),
                     ('imx8ulp', SocId.IMX8ULP),
                     ('imx8qm', SocId.IMX8QM),
                     ('imx8qxp', SocId.IMX8QXP),
                     ('imx93', SocId.IMX93),
                     ('imx95', SocId.IMX95),]

# dictionary of SoC name
soc_to_name = {SocId.IMX8MQ: "i.MX 8M Quad",
               SocId.IMX8MM: "i.MX 8M Mini",
               SocId.IMX8MN: "i.MX 8M Nano",
               SocId.IMX8MP: "i.MX 8M Plus",
               SocId.IMX8ULP: "i.MX 8ULP",
               SocId.IMX8QM: "i.MX 8QuadMax",
               SocId.IMX8QXP: "i.MX 8QuadXPlus",
               SocId.IMX95: "i.MX 95",
               SocId.IMX93: "i.MX 93", }

# dictionary of SoC features
soc_has_feature = {
    SocId.IMX8MQ:
        {Feature.GPU2D: False, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX8MM:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX8MN:
        {Feature.GPU2D: False, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX8MP:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: True, },
    SocId.IMX8ULP:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX8QM:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX8QXP:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: False, },
    SocId.IMX95:
        {Feature.GPU2D: True, Feature.GPU3D: True, Feature.NPU: True, },
    SocId.IMX93:
        {Feature.GPU2D: True, Feature.GPU3D: False, Feature.NPU: True, }, }


class Imx():

    def __init__(self):
        """Provide helpers to query running i.MX devices properties.
        """
        res = subprocess.run(['uname', '-a'],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out = res.stdout.decode()

        r = re.compile("^\s*(\S+)\s+(\S+)")  # noqa
        m = r.search(out)
        if m is None:
            raise ValueError(f"could not detect machine name in [{out}]")
        machine = m[2]

        self.soc = SocId.UNKNOWN
        for tuple in mach_regex_to_soc:
            r = re.compile(tuple[0])
            m = r.search(machine)
            if m is not None:
                self.soc = tuple[1]

        if not (self.is_imx8() or self.is_imx93() or self.is_imx95()):
            raise ValueError(f"unknown imx family [{self.soc}]")

        if self.soc == SocId.UNKNOWN:
            raise ValueError(f"unknown machine name [{machine}]")

    def id(self):
        return self.soc

    def name(self):
        try:
            return soc_to_name[self.soc]
        except BaseException as e:
            raise ValueError(f"unknown soc name [{self.soc}]")

    def has_gpu2d(self):
        return soc_has_feature[self.soc][Feature.GPU2D]

    def has_gpu3d(self):
        return soc_has_feature[self.soc][Feature.GPU3D]

    def has_gpuml(self):
        if self.soc == SocId.IMX8MM or self.soc == SocId.IMX8ULP:
            return False  # GPU not supported for ML acceleration
        else:
            return self.has_gpu3d()

    def has_npu(self):
        return soc_has_feature[self.soc][Feature.NPU]

    def has_gpu_vsi(self):
        return self.has_gpuml() and self.is_imx8()

    def has_npu_vsi(self):
        return self.soc == SocId.IMX8MP

    def has_npu_ethos(self):
        return self.soc == SocId.IMX93

    def has_npu_neutron(self):
        return self.soc == SocId.IMX95

    def has_g2d(self):
        return self.is_imx8() and self.soc != SocId.IMX8MQ or self.is_imx95()

    def has_pxp(self):
        return self.soc == SocId.IMX93

    def is_imx8(self):
        imx8 = [SocId.IMX8MQ, SocId.IMX8MM, SocId.IMX8MN, SocId.IMX8MP,
                SocId.IMX8QM, SocId.IMX8QXP, SocId.IMX8ULP]
        return self.soc in imx8

    def is_imx93(self):
        imx93 = [SocId.IMX93]
        return self.soc in imx93

    def is_imx95(self):
        imx95 = [SocId.IMX95]
        return self.soc in imx95


if __name__ == '__main__':
    logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.INFO)
    imx = Imx()
    logging.info(f"id() {imx.id()}")
    logging.info(f"name() {imx.name()}")
    logging.info(f"is_imx8() {imx.is_imx8()}")
    logging.info(f"is_imx93() {imx.is_imx93()}")
    logging.info(f"is_imx95() {imx.is_imx95()}")
    logging.info(f"has_gpu2d() {imx.has_gpu2d()}")
    logging.info(f"has_gpu3d() {imx.has_gpu3d()}")
    logging.info(f"has_gpuml() {imx.has_gpuml()}")
    logging.info(f"has_npu() {imx.has_npu()}")
    logging.info(f"has_gpu_vsi() {imx.has_gpu_vsi()}")
    logging.info(f"has_npu_vsi() {imx.has_npu_vsi()}")
    logging.info(f"has_npu_ethos() {imx.has_npu_ethos()}")
    logging.info(f"has_g2d() {imx.has_g2d()}")
    logging.info(f"has_pxp() {imx.has_pxp()}")
