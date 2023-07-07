/**
 * Copyright 2023 NXP
 * SPDX-License-Identifier: BSD-3-Clause 
 */ 

/**
 * NNstreamer application for image classification using tensorflow-lite. 
 * The model used is mobilenet_v1_1.0_224.tflite which can be retrieved from https://github.com/nxp-imx/nxp-nnstreamer-examples/blob/main/downloads/download.ipynb
 *  
 * Pipeline:
 * v4l2src -- tee -- textoverlay -- autovideosink
 *             |
 *             --- imxvideoconvert -- tensor_converter -- tensor_transform -- tensor_filter -- tensor_decoder
 */

#include <map>

#include "common_utils.hh"


/* Input from camera device */
const int CAMERA_INPUT_WIDTH = 640;
const int CAMERA_INPUT_HEIGHT = 480;
const int CAMERA_INPUT_FRAMERATE = 30;

/* Input from model */
const int MODEL_INPUT_WIDTH = 224;
const int MODEL_INPUT_HEIGHT = 224;

/* Specify the right path to your models */
const char *PATH_TO_CLASSIFICATION = "/tmp/models/classification";
const char *LABEL_NAME = "labels_mobilenet_quant_v1_224.txt";

/* Models used for different backends */
const char *MODEL_CPU = "mobilenet_v1_1.0_224.tflite";
const char *MODEL_GPU_VSI = "mobilenet_v1_1.0_224.tflite";
const char *MODEL_NPU_VSI = "mobilenet_v1_1.0_224_quant_uint8_float32.tflite";
const char *MODEL_NPU_ETHOS = "mobilenet_v1_1.0_224_quant_uint8_float32_vela.tflite";


/** 
 * @brief i.MX Backend enumeration.
 */
enum Backend
{
    CPU,
    GPU,
    NPU,
};


/** 
 * @brief Dictionary of Backend identification.
 */ 
std::map<std::string, int> inference_hardware_backend = {
    {"CPU", CPU},
    {"GPU", GPU},
    {"NPU", NPU},
};


/**
 * @brief Data structure for TensorFlow Lite model info.
 */
typedef struct
{
    gchar *model_path; 
    gchar *label_path;
} tflite_model_info_t;
              

/**
 * @brief Data structure for app.
 */
typedef struct
{
    GMainLoop *loop; 
    GstElement *pipeline;
    gboolean running;
    tflite_model_info_t tflite_info; 
} app_data_t;


/* Data for pipeline and result */
static app_data_t g_app;


/**
 * @brief Application to process a camera video stream with an object classification model.
 */
class ClassificationExample
{
    private:
        gchar *str_pipeline;
        gchar *imx_acceleration;  
        gchar *tflite_model_name;

        TensorCustomDataGenerator tensor_custom_data_generator{};
        tensor_data_t tensor_data;
        GstVideoImx gstvideoimx{};

        int soc_id;
        bool app_data_allocated = false;
                
    public:
        ClassificationExample(int argc, char **argv)
        {
            /* init app variable */
            g_app.running = FALSE;
            g_app.loop = NULL;
            g_app.pipeline = NULL;

            this->str_pipeline = NULL;
            this->imx_acceleration = NULL;
            this->tflite_model_name = NULL;

            /* init Gstreamer */
            gst_init(&argc, &argv);
        }
        

        /**
         * @brief Create and run the pipeline.
         * 
         * @param backend: second argument at runtime corresponding to backend use.
         * @param camera_device: camera device used
         * @param imx: i.MX used.
         */
        void run(char *backend, char *camera_device, GError *error, _GOptionContext *context, imx::Imx imx, bool error_allocated)
        {
            log_info("Start app...\n");

            store_vx_graph_compilation(imx);

            /* init pipeline for imx acceleration */
            this->imx_acceleration = g_strdup(gstvideoimx.videoscale_to_rgb(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT).c_str());
            
            /* init model, label path and setup tensor configuration and backend */
            try
            {
                set_tensor_filter_config(backend, imx);
                tflite_init_info(&g_app.tflite_info, PATH_TO_CLASSIFICATION, this->tflite_model_name, LABEL_NAME);
            }
            catch(const std::exception& e)
            {
                log_error("%s\n", e.what());
                log_info("close app...\n");
                free_data(&g_app.tflite_info, backend, camera_device, error, context, error_allocated);
                return;
            }

            /* main loop */
            g_app.loop = g_main_loop_new(NULL, FALSE);

            /* init pipeline */
            this->str_pipeline = g_strdup_printf("v4l2src name=cam_src device=%s num-buffers=-1 ! "
                "video/x-raw,width=%s,height=%s,framerate=%s/1 ! "
                "tee name=t ! "
                "queue name=thread-nn max-size-buffers=2 leaky=2 ! "
                "%s " /* videoscale to rgb pipeline with i.MX accelerator */
                "tensor_converter ! "
                "%s" /* tensor_transform segment pipeline */
                "tensor_filter framework=tensorflow-lite  model=%s %s ! " /* tensor_filter custom options pipeline */
                "tensor_decoder mode=image_labeling option1=%s ! "
                "overlay.text_sink "
                "t. ! queue name=thread-img max-size-buffers=2 leaky=2 ! "
                "textoverlay name=overlay font-desc=\"Sans, 24\" ! "
                "autovideosink sync=false ", 
                camera_device, std::to_string(CAMERA_INPUT_WIDTH).c_str(), std::to_string(CAMERA_INPUT_HEIGHT).c_str(), 
                std::to_string(CAMERA_INPUT_FRAMERATE).c_str(), this->imx_acceleration, this->tensor_data.tensor_transform,
                g_app.tflite_info.model_path, this->tensor_data.tensor_filter_custom, g_app.tflite_info.label_path);
            
            log_debug("%s\n\n", this->str_pipeline);

            this->app_data_allocated = true;

            g_app.pipeline = gst_parse_launch(this->str_pipeline, NULL);

            /* start pipeline */
            gst_element_set_state(g_app.pipeline, GST_STATE_PLAYING);
            g_app.running = TRUE;
            
            /* shutdowm pipeline with SIGINT signal */
            g_unix_signal_add(SIGINT, sigint_signal_handler, g_app.loop);

            /* run main loop */
            g_main_loop_run(g_app.loop);

            /* quit when received eos or error message */
            g_app.running = FALSE;

            /* end pipeline */
            gst_element_set_state(g_app.pipeline, GST_STATE_READY);
            gst_element_set_state(g_app.pipeline, GST_STATE_NULL);

            log_info("close app...\n");

            free_data(&g_app.tflite_info, backend, camera_device, error, context, error_allocated);
        }

        
        /**
         * @brief Closing the main loop with SIGINT signal.
         * 
         * @param user_data: data on main loop.
         * @return confirmation of main loop closure.
         */
        static gboolean sigint_signal_handler(gpointer user_data)
        {
            GMainLoop *loop = (GMainLoop *) user_data;

            log_info("Closing the main loop\n");
            g_main_loop_quit(loop);

            return TRUE;
        }


        /**
         * @brief Setup tensor configuration and select backend use.
         * 
         * @param backend: second argument at runtime corresponding to backend use.
         * @param imx: i.MX used.
         */
        void set_tensor_filter_config(char *backend, imx::Imx imx)
        {   
            switch (select_backend(backend)) {
                case CPU:
                    this->tflite_model_name = g_strdup(MODEL_CPU);
                    this->tensor_data = tensor_custom_data_generator.cpu();
                    break;

                case GPU:
                    if(imx.is_imx8())
                    {
                        this->tflite_model_name = g_strdup(MODEL_GPU_VSI);
                        this->tensor_data = tensor_custom_data_generator.gpu_vsi();
                    }
                    else
                    {
                        throw std::runtime_error("cannot used this backend with " + imx.soc_name());
                    }
                    break;

                case NPU:
                    if (imx.is_imx8() && imx.has_npu())
                    {
                        this->tflite_model_name = g_strdup(MODEL_NPU_VSI);
                        this->tensor_data = tensor_custom_data_generator.npu_vsi();
                    }         
                    else if(imx.is_imx9() && imx.has_npu())
                    {
                        this->tflite_model_name = g_strdup(MODEL_NPU_ETHOS);
                        this->tensor_data = tensor_custom_data_generator.npu_ethos();
                    }           
                    break;

                default:
                    if (imx.is_imx8() && imx.has_npu())
                    {
                        this->tflite_model_name = g_strdup(MODEL_NPU_VSI);
                        this->tensor_data = tensor_custom_data_generator.npu_vsi();
                    }         
                    else if(imx.is_imx9() && imx.has_npu())
                    {
                        this->tflite_model_name = g_strdup(MODEL_NPU_ETHOS);
                        this->tensor_data = tensor_custom_data_generator.npu_ethos();
                    }  
                    break;
            }
        }

        /**
         * @brief Check if the backend exists. 
         * 
         * @param backend: second argument at runtime corresponding to backend use.
         * @return the selected backend or -1 if he doesn't exist.
         */
        int select_backend(char *backend)
        {
            for(auto& pair : inference_hardware_backend)
            {
                std::string key = pair.first;
                if(backend == key)
                {
                    return inference_hardware_backend[backend];
                }
            }  
            return -1;
        }


        /**
         * @brief Check tflite model and load labels.
         * 
         * @param tflite_info: model and label tensor information.
         * @param path: path to tensor information.
         * @param tflite_model_name: tensor model used depending on the backend.
         * @param tflite_label_name: label used for tensor decoder
         */
        void tflite_init_info(tflite_model_info_t *tflite_info, const gchar *path, const gchar *tflite_model_name, const gchar *tflite_label_name)
        {
            tflite_info->model_path = g_strdup_printf("%s/%s", path, tflite_model_name);
            tflite_info->label_path = g_strdup_printf("%s/%s", path, tflite_label_name);

            open_file(tflite_info->model_path);
            open_file(tflite_info->label_path);
        }


        /**
         * @brief Check if the file exists.
         * 
         * @param file_path: path to file.
         */
        void open_file(gchar *file_path)
        {
            std::ifstream file(file_path);
            if (file.is_open()) 
            {
                file.close();
            } 
            else 
            {
                std::string message;
                message = "cannot find ";
                message += file_path;
                throw std::ifstream::failure(message);
            }  
        }


        /**
         * @brief Free data used in the application.
         * 
         * @param tflite_info: model and label tensor information
         */
        void free_data(tflite_model_info_t *tflite_info, char *backend, char *camera_device, GError *error, GOptionContext *context, bool error_allocated)
        {
            if (this->app_data_allocated)
            {
                g_main_loop_unref(g_app.loop);
                g_app.loop = NULL;

                gst_object_unref(g_app.pipeline);
                g_app.pipeline = NULL;

                g_free(tflite_info->model_path);
                tflite_info->model_path = NULL;

                g_free(tflite_info->label_path);
                tflite_info->label_path = NULL;

                g_free(this->str_pipeline);
                g_free(this->imx_acceleration);
                g_free(this->tflite_model_name);
            }
            
            if (error_allocated)
            {       
                g_error_free(error);
            }

            g_option_context_free(context);
            g_free(backend);
            g_free(camera_device);
        }
};


/**
 * @brief Specify the backend as the second argument and 
 *        camera device can be specified as the third argument.
*/
int main(int argc, char **argv)
{

    try {
        imx::Imx imx{};
        ClassificationExample classification_example(argc, argv);

        bool error_allocated = false;

        /* selecting the right camera by default */
        int soc_id = imx.soc_id();

        gchar *camera_device = (soc_id == imx::IMX8MP) ? g_strdup("/dev/video3") : g_strdup("/dev/video0");
        gchar *backend = g_strdup("NPU");

        GOptionContext *context = g_option_context_new ("-Option Parser");
        GError *error = NULL;  

        GOptionEntry entries[] =
        {
            { "backend", 'b', 0, G_OPTION_ARG_STRING, &backend, "Use the selected backend (CPU,GPU,NPU)", NULL },
            { "camera_device", 'c', 0, G_OPTION_ARG_STRING, &camera_device, "Use the selected camera device (/dev/video{number})" , NULL },
            { NULL }
        };
        
        g_option_context_add_main_entries (context, entries, NULL);

        if (!g_option_context_parse (context, &argc, &argv, &error))
        {
            error_allocated = true;
            log_error("option parsing failed: %s\n", error->message);
            classification_example.free_data(&g_app.tflite_info, backend, camera_device, error, context, error_allocated);
            return 1;
        }

        classification_example.run(backend, camera_device, error, context, imx, error_allocated);
    }
    catch(const std::runtime_error& e)
    {
        log_error("Exception caught: %s\n", e.what());
    }

    return 0;
}