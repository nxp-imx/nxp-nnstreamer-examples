/**
 * Copyright 2023 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef IMX_DEVICES_HH
#define IMX_DEVICES_HH

#include <array>
#include <fstream>
#include <iostream>

#include "logging.hh"


std::string SOC_ID_PATH = "/sys/devices/soc0/soc_id";

const int NUMBER_OF_SOC = 8;
const int NUMBER_OF_FEATURE = 3;

namespace imx {

    /** 
     * @brief i.MX devices enumeration.
     */
    enum iMXSocId
    {
        IMX8MQ,
        IMX8MM,
        IMX8MN,
        IMX8MP,
        IMX8ULP,
        IMX8QM,
        IMX8QXP,
        IMX93,
        UNKNOWN,
    };


    /** 
     * @brief i.MX Features enumeration.
     */
    enum iMXHwAccelFeature
    {
        GPU2D,
        GPU3D,
        NPU,
    };


    /**
     * @brief Dictionary for SoC identification.
     */
    const std::array<std::string, NUMBER_OF_SOC> soc_identification = {
        "i.MX8MQ",
        "i.MX8MM",
        "i.MX8MN",
        "i.MX8MP",
        "i.MX8ULP",
        "i.MX8QM",
        "i.MX8QXP",
        "i.MX93",
    };


    /** 
     * @brief Array of SoC name.
     */
    const std::array<std::string, NUMBER_OF_SOC> soc_to_name = {
        "i.MX 8M Quad",
        "i.MX 8M Mini",
        "i.MX 8M Nano",
        "i.MX 8M Plus",
        "i.MX 8ULP",
        "i.MX 8QuadMax",
        "i.MX 8QuadXPlus",
        "i.MX 93",
    };


    /** 
     * @brief Array of SoC features.
     */
    const std::array<std::array<bool, NUMBER_OF_FEATURE>, NUMBER_OF_SOC> soc_has_feature = {{
        [IMX8MQ] = {{[GPU2D] = false, [GPU3D] = true, [NPU] = false}},
        [IMX8MM] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
        [IMX8MN] = {{[GPU2D] = false, [GPU3D] = true, [NPU] = false}},
        [IMX8MP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = true}},
        [IMX8ULP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
        [IMX8QM] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
        [IMX8QXP] = {{[GPU2D] = true, [GPU3D] = true, [NPU] = false}},
        [IMX93] = {{[GPU2D] = true, [GPU3D] = false, [NPU] = true}},
    }};


    /**
     * @brief Discovers some hardware acceleration features of an i.MX SoC (presence of 2D/3D GPU, presence of NPU...)
     */
    class Imx
    {
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
                std::ifstream file_soc_id(SOC_ID_PATH);
                this->soc = UNKNOWN;

                if(file_soc_id)
                {
                    std::string soc_id;

                    getline(file_soc_id, soc_id);

                    for (int i=0;i<NUMBER_OF_SOC;i++)
                    {
                        if(soc_identification[i] == soc_id)
                        {
                            this->soc = i;
                        }
                    }

                    file_soc_id.close(); 
                }
                else
                {
                    log_error("Unable to open soc_id file\n");
                    file_soc_id.close(); 
                }

                if(this->soc == UNKNOWN)
                {
                    throw std::runtime_error("unknown machine name\n");
                }
            }

            int soc_id()
            {
                return this->soc;
            }

            std::string soc_name()
            {   
                return soc_to_name[this->soc];
            }

            bool has_gpu2d()
            {
                return soc_has_feature[this->soc][GPU2D];
            }

            bool has_gpu3d()
            {
                return soc_has_feature[this->soc][GPU3D];
            }

            bool has_gpuml()
            {
                /* GPU not supported for ML acceleration */
                if(this->soc == IMX8MM || this->soc == IMX8ULP)
                {
                    return false;   
                }
                else 
                {
                    return this->has_gpu3d();
                }
            }

            bool has_npu()
            {
                return soc_has_feature[this->soc][NPU];
            }

            bool has_gpu_vsi()
            {
                return this->has_gpuml() && this->is_imx8();
            }

            bool has_npu_vsi()
            {
                return this->soc == IMX8MP;
            }

            bool has_npu_ethos()
            {
                return this->soc == IMX93;
            }

            bool has_g2d()
            {
                return this->is_imx8() && this->soc != IMX8MQ;
            }

            bool has_pxp()
            {
                switch (this->soc)
                {
                    case IMX93:
                        return true;
                        break;
                    
                    default:
                        return false;
                        break;
                }
            }

            bool is_imx8()
            {
                switch (this->soc)
                {
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

            bool is_imx9()
            {
                switch (this->soc)
                {
                    case IMX93:
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