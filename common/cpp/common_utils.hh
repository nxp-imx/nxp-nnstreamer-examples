/**
 * Copyright 2023 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

#ifndef COMMON_UTILS_HH
#define COMMON_UTILS_HH

#include <glib.h>
#include <glib-unix.h>
#include <gst/gst.h>
#include <thread>

#include "imx_devices.hh"


/**
 * @brief Data structure for tensor.
 */
typedef struct
{
    gchar* tensor_filter_custom;
    gchar* tensor_transform;
} tensor_data_t;


/**
 * @brief Create pipeline segments for the various accelerators.
 */
class GstVideoImx
{
    private:
        imx::Imx imx{};

    public:
        GstVideoImx() = default;

        /**
         * @brief Create pipeline segment for accelerated video formatting and csc.
         * 
         * @param format: GStreamer video format.
         * @param width: output video width after rescale.
         * @param  height: output video height after rescale.
         * @return GStreamer pipeline segment string.
         */
        std::string videoscale_to_format(std::string format, int width, int height)
        {
            std::string cmd;

            if(this->imx.has_g2d())
            {
                cmd = "imxvideoconvert_g2d ! ";
            }
            else if(this->imx.has_pxp())
            {
                cmd = "imxvideoconvert_pxp ! ";
            }
            else 
            {
                cmd = "videoscale ! videoconvert ! ";
            }

            if(width != 0 && height != 0)
            {
                cmd += "video/x-raw,width=" + std::to_string(width) + ",";
                cmd += "height=" + std::to_string(height) + ",format=" + format + " ! ";
            }
            else
            {
                /* no acceleration */
                cmd += "video/x-raw,format=" + format + " ! ";
            }

            return cmd;
        }


        /**
         * @brief Create pipeline segment for accelerated video scaling and
         *        conversion to RGB format.
         * 
         * @param width: output video width after rescale.
         * @param height: output video height after rescale.
         * @return GStreamer pipeline segment string.
         */
        std::string videoscale_to_rgb(int width, int height)
        {
            std::string cmd;

            if(this->imx.has_g2d())
            {
                /**
                 * imxvideoconvert_g2d does not support RGB sink
                 * and use CPU to convert BGR to RGB
                 */ 
                cmd = this->videoscale_to_format("RGBA", width, height);
                cmd += "videoconvert ! video/x-raw,format=RGB ! ";
            }
            else if(this->imx.has_pxp())
            {
                /** 
                 * imxvideoconvert_pxp does not support RGB sink
                 * use acceleration to BGR
                 */
                cmd = this->videoscale_to_format("BGR", width, height);
                cmd += "videoconvert ! video/x-raw,format=RGB ! "; 
            }
            else
            {
                /* no acceleration */
                cmd = this->videoscale_to_format("RGB", width, height);
            }

            return cmd;
        }


        /**
         * @brief Create pipeline segment for accelerated video cropping and
         *        conversion to RGB format.
         * 
         * @param videocrop_name: GStreamer videocrop element name.
         * @param width: output video width after rescale.
         * @param height: output video height after rescale.
         * @param top: top pixels to be cropped - may be setup via property later.
         * @param bottom: bottom pixels to be cropped - may be setup via property later.
         * @param left: left pixels to be cropped - may be setup via property later.
         * @param right: right pixels to be cropped - may be setup via property later.
         * 
         * @return GStreamer pipeline segment string.
         */
        std::string videocrop_to_rgb(std::string videocrop_name, int width, int height, int top, int bottom, int left, int right)
        {
            std::string cmd;

            cmd = "videocrop name=" + videocrop_name + " ";

            if(top != 0)
            {
                cmd += "top=" + std::to_string(top) + " ";
            }
            if(bottom != 0)
            {
                cmd += "bottom=" + std::to_string(bottom) + " ";
            }
            if(left != 0)
            {
                cmd += "left=" + std::to_string(left) + " ";
            }
            if(right != 0)
            {
                cmd += "right=" + std::to_string(right) + " ";
            }
            
            cmd += "! ";

            cmd += this->videoscale_to_rgb(width, height);

            return cmd;
        }


        /**
         * @brief Create pipeline segment for accelerated video mixing.
         * 
         * @param latency: time for a capture to reach the sink, default 0.
         * @return GStreamer pipeline segment string.
         */
        std::string video_compositor(int latency = 0)
        {
            std::string cmd;
            std::string name = "mix";
            std::string nnst_stream = "sink_0";
            std::string main_stream = "sink_1";

            if(this->imx.has_g2d())
            {
                cmd = "imxcompositor_g2d name=" + name + " ";
                cmd += nnst_stream + "::zorder=2 ";
                cmd += main_stream + "::zorder=1 ";
            }
            else if(this->imx.has_pxp())
            {
                /** 
                 * imxcompositor_pxp does not support RGBA sink
                 * and use CPU to convert RGBA to RGB
                 */ 
                cmd = "imxcompositor_pxp name=" + name + " ";
                cmd += nnst_stream + "::zorder=2 ";
                cmd += main_stream + "::zorder=1 ";
                cmd += nnst_stream + "::alpha=0.3 ";
            }
            else
            {
                /*  no acceleration */
                cmd = "compositor name=" + name + " ";
            }

            if(latency != 0)
            {
                cmd += "latency=" + std::to_string(latency) + " min-upstream-latency=" + std::to_string(latency) + " ";
            }

            cmd += "! ";

            return cmd;
        }
};


/**
 * @brief Store on disk .nb files that contains the result of the OpenVX graph compilation
          This feature is only available for iMX8 platforms to get the warmup time only once
 * 
 * @param imx: i.MX used.
 */
void store_vx_graph_compilation(imx::Imx imx)
{
    bool is_vsi_platform = imx.has_npu_vsi() || imx.has_gpu_vsi();

    if(is_vsi_platform)
    {
        setenv("VIV_VX_ENABLE_CACHE_GRAPH_BINARY","1",1);
        char *path = getenv("HOME");
        setenv("VIV_VX_CACHE_BINARY_GRAPH_DIR",path,1);
    }
}


/**
 * @brief Create pipeline segments for customized tensor data and 
 *        set up USE_GPU_INFERENCE, an environment variable used for the GPU backend.
 */
class TensorCustomDataGenerator
{
    private:
        tensor_data_t tensor_data;
        char *num_threads;

    public:
        TensorCustomDataGenerator()
        {
            this->tensor_data.tensor_filter_custom = NULL;
            this->tensor_data.tensor_transform =  NULL;
            this->num_threads = g_strdup_printf("%d",std::thread::hardware_concurrency()); 
        }

        tensor_data_t cpu()
        {
            this->tensor_data.tensor_filter_custom = g_strdup_printf("custom=Delegate:XNNPACK,NumThreads:%s", this->num_threads);
            this->tensor_data.tensor_transform = g_strdup("tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! ");
            return tensor_data;
        }

        tensor_data_t gpu_vsi()
        {
            this->tensor_data.tensor_filter_custom = g_strdup("custom=Delegate:External,ExtDelegateLib:libvx_delegate.so");
            this->tensor_data.tensor_transform = g_strdup("tensor_transform mode=arithmetic option=typecast:float32,add:-127.5,div:127.5 ! ");
            setenv("USE_GPU_INFERENCE","1",1);
            return tensor_data;
        }

        tensor_data_t npu_vsi()
        {
            this->tensor_data.tensor_filter_custom = g_strdup("custom=Delegate:External,ExtDelegateLib:libvx_delegate.so");
            this->tensor_data.tensor_transform = g_strdup("");
            setenv("USE_GPU_INFERENCE","0",1); 
            return tensor_data;
        }

        tensor_data_t npu_ethos()
        {
            this->tensor_data.tensor_filter_custom = g_strdup("custom=Delegate:External,ExtDelegateLib:libethosu_delegate.so");
            this->tensor_data.tensor_transform = g_strdup("");
            return tensor_data;
        }
};
#endif
