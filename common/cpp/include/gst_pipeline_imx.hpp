/**
 * Copyright 2024-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPP_GST_PIPELINE_IMX_H_
#define CPP_GST_PIPELINE_IMX_H_

#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include <cairo.h>
#include <vector>

#include "imx_devices.hpp"


/**
 * @brief Data structure for app.
 */
typedef struct {
  GstElement *gstPipeline;
  GstBus *bus;
  bool playing;
  float FPS;
  std::vector<std::string> filterNames;
} AppData;


/**
 * @brief GStreamer queue leaky options.
 */
enum class GstQueueLeaky {
  no,
  upstream,
  downstream,
};


/**
 * @brief GStreamer queue options.
 */
typedef struct {
  std::string queueName = "";
  int maxSizeBuffer = -1;
  GstQueueLeaky leakType = GstQueueLeaky::no;
} GstQueueOptions;


/**
 * @brief Performances options.
 */
typedef struct {
  bool freq;
  bool temp;
} Performance;


/**
 * @brief Outline text for Cairo.
 */
void outlineText(cairo_t* cr,
                 int x,
                 int y,
                 std::string txt,
                 std::string color);


/**
 * @brief Setup and run GStreamer pipeline.
 */
class GstPipelineImx {
  private:
    std::string strPipeline;
    AppData gApp {};
    static inline bool save = false;
    static inline Performance hasPerf = {false, false};
    static inline GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    static inline int pipeCount = 0;
    static inline int runCount = 0;
    static inline std::vector<std::string> namesVector;
    static inline std::vector<float> infVector;
    static inline float perfFontSize = 0;
    static inline std::string perfColor = "";

  public:
    static gboolean pipePerfCallback(gpointer user_data);

    static gboolean infPerfCallback(gpointer user_data);

    void parse(int argc, char **argv, char *graphPath);

    void run();

    void freeData();

    void addBranch(const std::string &teeName, const GstQueueOptions &options);

    static void busCallback(GstBus* bus,
                            GstMessage* message,
                            gpointer user_data);

    static gboolean sigintSignalHandler(gpointer user_data);

    void doInParallel(const std::string &teeName);

    void addToPipeline(const std::string &cmd) { strPipeline.append(cmd); }

    AppData getAppData() const { return gApp; }

    void linkToTextOverlay(const std::string &gstName);

    void addTensorSink(const std::string &gstName, const bool &qos=true);

    void setSave(const bool &save);

    GstElement* getElement(const std::string &gstName);

    template<typename F>
    void connectToElementSignal(const std::string &gstName,
                                F callback,
                                const std::string &signal,
                                gpointer data)
    {
      GstElement *element = getElement(gstName);
      if (element) {
        u_long handleID = g_signal_connect(element,
                                           signal.c_str(),
                                           G_CALLBACK(callback),
                                           data);
        if (handleID <= 0) {
          log_error("could not connect %s\n", gstName.c_str());
          exit(-1);
        }
      } else {
        log_error("Could not get %s\n", gstName.c_str());
        exit(-1);
      }
    }

    void enablePerfDisplay(const bool &frequency,
                           const bool &temporal,
                           const float &fontSize,
                           const std::string &color="");

    Performance isPerfAvailable() const
    {
      return hasPerf;
    }

    static void perfDrawCallback(GstElement* overlay,
                                 cairo_t* cr,
                                 guint64 timestamp,
                                 guint64 duration,
                                 gpointer user_data);

    void addFilterName(std::string gstName);
};
#endif