/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef CPP_IMX_DEVICES_H_
#define CPP_IMX_DEVICES_H_

#include <array>
#include <fstream>
#include <iostream>

#include "logging.hpp"

#define NUMBER_OF_SOC       9
#define NUMBER_OF_FEATURE   3
#define SOC_ID_PATH         "/sys/devices/soc0/soc_id"

namespace imx {

  /** 
   * @brief i.MX devices enumeration.
   */
  enum iMXSocId {
    IMX8MQ,
    IMX8MM,
    IMX8MN,
    IMX8MP,
    IMX8ULP,
    IMX8QM,
    IMX8QXP,
    IMX93,
    IMX95,
    UNKNOWN,
  };


  /** 
   * @brief i.MX Features enumeration.
   */
  enum iMXHwAccelFeature {
    GPU2D,
    GPU3D,
    NPU,
  };


  /**
   * @brief Dictionary for SoC identification.
   */
  const std::array<std::string, NUMBER_OF_SOC> socDictionnary = {
    "i.MX8MQ",
    "i.MX8MM",
    "i.MX8MN",
    "i.MX8MP",
    "i.MX8ULP",
    "i.MX8QM",
    "i.MX8QXP",
    "i.MX93",
    "i.MX95",
  };


  /** 
   * @brief Array of SoC name.
   */
  const std::array<std::string, NUMBER_OF_SOC> socNameArray = {
    "i.MX 8M Quad",
    "i.MX 8M Mini",
    "i.MX 8M Nano",
    "i.MX 8M Plus",
    "i.MX 8ULP",
    "i.MX 8QuadMax",
    "i.MX 8QuadXPlus",
    "i.MX 93",
    "i.MX 95",
  };


  /** 
   * @brief Array of SoC features.
   */
  const std::array<std::array<bool, NUMBER_OF_FEATURE>, NUMBER_OF_SOC> socHasFeature = {{
    [IMX8MQ] = {{[GPU2D] = false, [GPU3D] = true, [NPU] = false}},
    [IMX8MM] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
    [IMX8MN] = {{[GPU2D] = false, [GPU3D] = true, [NPU] = false}},
    [IMX8MP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = true}},
    [IMX8ULP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
    [IMX8QM] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
    [IMX8QXP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
    [IMX93] = {{[GPU2D] = false, [GPU3D] = false, [NPU] = true}},
    [IMX95] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = true}},
  }};


  /** 
   * @brief i.MX Backend enumeration.
   */
  enum class Backend {
    CPU,
    GPU,
    NPU,
  };


  /**
   * @brief Discovers some hardware acceleration features of an i.MX SoC (presence of 2D/3D GPU, presence of NPU...)
   */
  class Imx {
    private:
      int soc;

    public:
      /**
       * @brief Constructor to detect i.MX device.
       * 
       * @throw Generates an error if the machine name is unknown.
       */
      Imx() 
      {   
        std::ifstream fileSocId(SOC_ID_PATH);
        soc = UNKNOWN;

        if(fileSocId) {
          std::string socId;
          getline(fileSocId, socId);

          for (int i=0;i<NUMBER_OF_SOC;i++) {
            if(socDictionnary[i] == socId)
              soc = i;
          }

          fileSocId.close(); 
        } else {
          log_error("Unable to open socId file\n");
          fileSocId.close(); 
        }

        if(soc == UNKNOWN) {
          log_error("unknown machine name\n");
          exit(-1);
        }
      }

      int socId() const { return soc; }

      std::string socName() const { return socNameArray[soc]; }

      bool hasGPU2d() { return socHasFeature[soc][GPU2D]; }

      bool hasGPU3d() { return socHasFeature[soc][GPU3D]; }

      bool hasGPUml() 
      {
        /* GPU not supported for ML acceleration */
        if(soc == IMX8MM || soc == IMX8ULP) {
          return false;   
        } else  {
          return hasGPU3d();
        }
      }

      bool hasNPU() { return socHasFeature[soc][NPU]; }

      bool hasVsiGPU() { return hasGPUml() && isIMX8(); }

      bool hasVsiNPU() { return soc == IMX8MP; }

      bool hasEthosNPU() { return soc == IMX93; }

      bool hasNeutronNPU() { return soc == IMX95; }

      bool hasG2d() { return (isIMX8() && soc != IMX8MQ) || soc == IMX95; }

      bool hasPxP() 
      {
        switch (soc) {
          case IMX93:
            return true;
            break;

          default:
            return false;
            break;
        }
      }

      bool isIMX8() 
      {
        switch (soc) {
          case IMX8MQ:
          case IMX8MM:
          case IMX8MN:
          case IMX8MP:
          case IMX8ULP:
          case IMX8QM:
          case IMX8QXP:
            return true;
            break;

          default:
            return false;
            break;
        }
      }

      bool isIMX9() 
      {
        switch (soc) {
          case IMX93:
          case IMX95:
            return true;
            break;

          default:
            return false;
            break;
        }
      }
  };
}
#endif
