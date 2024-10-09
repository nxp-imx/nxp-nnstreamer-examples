/**
 * Copyright 2024 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gst_pipeline_imx.hpp"
#include <cmath>

/**
 * @brief Store on disk .nb files that contains the result of the OpenVX graph
 *        compilation. This feature is only available for iMX8 platforms to get
 *        the warmup time only once.
 * 
 * @param imx: i.MX used.
 * @param graphPath: store .nb files in provided path.
 */
void storeVxGraphCompilation(imx::Imx imx, char *graphPath)
{
  if(imx.socId() == imx::IMX8MP) {
    setenv("VIV_VX_ENABLE_CACHE_GRAPH_BINARY","1",1);
    setenv("VIV_VX_CACHE_BINARY_GRAPH_DIR",graphPath,1);
  }
}


/**
 * @brief Callback function to retrieve tensor_filter latency.
 */
gboolean GstPipelineImx::infPerfCallback(gpointer user_data)
{
  AppData *gApp = (AppData *) user_data;
  if (namesVector.size() == 0)
    return false;

  int latency;
  for (int i = 0; i < gApp->filterNames.size(); i++) {
    GstElement *filter = gst_bin_get_by_name(GST_BIN(gApp->gstPipeline), gApp->filterNames.at(i).c_str());
    g_object_get(G_OBJECT(filter), "latency", &latency, NULL);

    for (int j = 0; j < namesVector.size(); j++) {
      if (gApp->filterNames.at(i) == namesVector.at(j)) {
        if (infVector.size() == namesVector.size())
          infVector.at(j) = latency;
      }
    }
  }
  return true;
}


/**
 * @brief Callback function to retrieve pipeline duration.
 */
gboolean GstPipelineImx::pipePerfCallback(gpointer user_data)
{
  AppData *gApp = (AppData *) user_data;
  
  gchar *fps_msg;
  GstElement *wayland = gst_bin_get_by_name(GST_BIN(gApp->gstPipeline), "img_tensor");
  if (!wayland)
    return false;
  g_object_get(G_OBJECT(wayland), "last-message", &fps_msg, NULL);
  if (fps_msg != NULL) {
    std::string message = fps_msg;

    std::string fps;
    if (message.find("current:") != std::string::npos) {
      fps = message.substr(message.find("current:") + 9, 5);
      gApp->FPS = std::stof(fps);
    }
    if (message.find("fps:") != std::string::npos) {
      fps = message.substr(message.find("fps:") + 5, 5);
      gApp->FPS = std::stof(fps);
    }
  }
  return true;
}


/**
 * @brief Parse gst pipeline, and add bus signal watcher.
 * @param graphPath: store .nb files in provided path.
 */
void GstPipelineImx::parse(int argc, char **argv, char *graphPath)
{
  imx::Imx imx{};
  gst_init(&argc, &argv);
  log_info("Start app...\n");
  storeVxGraphCompilation(imx, graphPath);

  pipeCount += 1;

  /* main loop */
  log_debug("%s\n\n", strPipeline.c_str());
  gApp.gstPipeline = gst_parse_launch(strPipeline.c_str(), NULL);

  /* bus and message callback */
  gApp.bus = gst_element_get_bus(gApp.gstPipeline);
  gst_bus_add_signal_watch(gApp.bus);
  g_signal_connect(gApp.bus, "message", G_CALLBACK (busCallback), &gApp);

  /* shutdowm pipeline with SIGINT signal */
  g_unix_signal_add(SIGINT, sigintSignalHandler, &gApp);
}


/**
 * @brief Run app and pipeline.
 */
void GstPipelineImx::run()
{
  if ((hasPerf.freq == true) || (hasPerf.temp == true)) {
    if (gst_bin_get_by_name(GST_BIN(gApp.gstPipeline), "perf")) {
      std::vector<float> tempVector(namesVector.size());
      infVector = tempVector;
      connectToElementSignal("perf", perfDrawCallback, "draw", &gApp);
      g_timeout_add(50, pipePerfCallback, &gApp);
    }
    g_timeout_add(50, infPerfCallback, &gApp);
  }

  runCount += 1;

  /* start pipeline */
  gst_element_set_state(gApp.gstPipeline, GST_STATE_PLAYING);

  if (runCount == pipeCount) {
    /* run main loop, quit when received eos or error message */
    g_main_loop_run(loop);

    /* end pipeline */
    gst_element_set_state(gApp.gstPipeline, GST_STATE_NULL);

    log_info("close app...\n");

    freeData();
  }
}


/**
 * @brief Free data used in the application.
 */
void GstPipelineImx::freeData()
{
  if (loop) {
    g_main_loop_unref(loop);
    loop = NULL;
  }   

  if (gApp.bus) {
    gst_bus_remove_signal_watch(gApp.bus);
    gst_object_unref(gApp.bus);
    gApp.bus = NULL;
  }

  if (gApp.gstPipeline) {
    gst_object_unref(gApp.gstPipeline);
    gApp.gstPipeline = NULL;
  }
}


/**
 * @brief Add branch to tee pipe, with a queue.
 * 
 * @param teeName: name of GStreamer tee element.
 * @param options: structure for queue parameters.
 */
void GstPipelineImx::addBranch(const std::string &teeName, 
                               const GstQueueOptions &options)
{
  std::string cmdName;
  std::string cmdBuffMaxSize;
  std::string cmdLeak;

  if (options.queueName.length() != 0)
    cmdName = " name=" + options.queueName;
  if (options.maxSizeBuffer != -1)
    cmdBuffMaxSize = " max-size-buffers=" 
                     + std::to_string(options.maxSizeBuffer);
  if (options.leakType != GstQueueLeaky::no)
    cmdLeak = " leaky=" + std::to_string(static_cast<int>(options.leakType));

  std::string cmd = teeName + ". ! " + "queue" + cmdName
                    + cmdBuffMaxSize + cmdLeak +  " ! ";
  addToPipeline(cmd);
}


/**
 * @brief Callback to manage GStreamer bus.
 */
void GstPipelineImx::busCallback(GstBus* bus,
                                 GstMessage* message,
                                 gpointer user_data)
{
  AppData* gApp = (AppData *) user_data;
  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_ERROR: {
      GError *err;
      gchar *debugInfo;

      gst_message_parse_error(message, &err, &debugInfo);
      log_error("Error received from element %s: %s.\n",
                GST_OBJECT_NAME(message->src),
                err->message);
      log_error("Debugging information: %s.\n",
                debugInfo ? debugInfo : "none");
      g_error_free(err);
      g_free(debugInfo);
      log_debug("Closing the main loop.\n");
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_EOS: {
      log_debug("End-Of-Stream reached.\n");
      log_debug("Closing the main loop.\n");
      g_main_loop_quit(loop);
      break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
      GstState oldState, newState, pendingState;
      gst_message_parse_state_changed(message, &oldState, &newState, &pendingState);
      if (GST_MESSAGE_SRC(message) == GST_OBJECT(gApp->gstPipeline)) {
        log_debug("Pipeline state changed from %s to %s.\n",
            gst_element_state_get_name(oldState), gst_element_state_get_name(newState));

        gApp->playing = (newState == GST_STATE_PLAYING);
      }
    }
    default:
      break;
  }
}


/**
 * @brief Handle SIGINT signal to stop the application.
 */
gboolean GstPipelineImx::sigintSignalHandler(gpointer user_data)
{
  AppData* gApp = (AppData *) user_data;
  if (save == true) {
    int result = gst_element_send_event(gApp->gstPipeline, gst_event_new_eos());
    if (!result) {
      log_error("Couldn't send EOS event\n");
      exit(-1);
    }

    /* wait for EOS (blocks) */
    GstClockTime timeout = 3 * GST_SECOND;
    GstMessage *msg;

    msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(gApp->gstPipeline),
    timeout, GST_MESSAGE_EOS);

    log_debug("No EOS after 3 seconds!\n");
    log_debug("Closing the main loop.\n");
    g_main_loop_quit(loop);
  } else {
    log_debug("SIGINT signal detected.");
    log_debug("Closing the main loop.\n");
    g_main_loop_quit(loop);
  }

  return TRUE;
}


/**
 * @brief Add a tee pipe element to the pipeline to parallelize of tasks.
 * 
 * @param teeName: name of GStreamer tee element.
 */
void GstPipelineImx::doInParallel(const std::string &teeName)
{
  std::string cmd = "tee name=" + teeName + " ";
  addToPipeline(cmd);
}


/**
 * @brief Link text to textoverlay.
 * 
 * @param name: name of textoverlay element.
 */
void GstPipelineImx::linkToTextOverlay(const std::string &name)
{
  addToPipeline(name + ".text_sink ");
}


/**
 * @brief Link video to compositor.
 * 
 * @param name: name of compositor element.
 */
void GstPipelineImx::linkToVideoCompositor(const std::string &name)
{
  addToPipeline(name + ". ");
}


/**
 * @brief Add tensor_sink element to retrieve tensors.
 * 
 * @param name: name of tensor_sink element.
 */
void GstPipelineImx::addTensorSink(const std::string &name, const bool &qos)
{
  addToPipeline("tensor_sink name=" + name + " ");
  if (qos == false)
    addToPipeline("qos=false ");
}


/**
 * @brief Set save state.
 * 
 * @param save: if we save output or not.
 */
void GstPipelineImx::setSave(const bool &save)
{
  this->save = save;
}


/**
 * @brief Get element from parsed pipeline as a GstElement.
 * 
 * @param name: name of element.
 */
GstElement* GstPipelineImx::getElement(const std::string &name)
{
  return gst_bin_get_by_name(GST_BIN(gApp.gstPipeline), name.c_str());
}


/**
 * @brief Add performances display of pipeline and models inference.
 * 
 * @param frequency: if we display performances in frequency domain.
 * @param temporal: if we display performances in temporal domain.
 * @param fontSize: font size of displayed performances.
 * @param color: color of displayed performances.
 */
void GstPipelineImx::enablePerfDisplay(const bool &frequency,
                                       const bool &temporal,
                                       const int &fontSize,
                                       const std::string &color)
{
  perfColor = color;
  perfFontSize = fontSize;
  hasPerf = {frequency, temporal};
}


/**
 * @brief Outline text with Cairo.
 * 
 * @param cr: current state of the rendering device.
 * @param x: x position of text.
 * @param y: y position of text.
 * @param txt: Text to outline.
 * @param color: Text color.
 */
void outlineText(cairo_t* cr,
                 int x,
                 int y,
                 std::string txt,
                 std::string color)
{
  int red = 1;
  int green = 1;
  int blue = 1;
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 2);
  cairo_move_to(cr, x, y);
  cairo_text_path(cr, txt.c_str());
  cairo_stroke(cr);

  if ((color != "green")
      && (color != "blue")
      && (color != "red")
      && (color != "black")
      && (color != "")) {
    log_error("Choose color between green, blue, red, and black\n");
    exit(-1);
  }
  if (color == "green") {
    red = 0;
    blue = 0;
  }
  if (color == "blue") {
    red = 0;
    green = 0;
  }
  if (color == "red") {
    green = 0;
    blue = 0;
  }
  if (color == "black") {
    red = 0;
    green = 0;
    blue = 0;
  }

  cairo_set_source_rgb(cr, red, green, blue);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, txt.c_str());
}


/**
 * @brief Callback which draw performances on screen.
 */
void GstPipelineImx::perfDrawCallback(GstElement* overlay,
                                      cairo_t* cr,
                                      guint64 timestamp,
                                      guint64 duration,
                                      gpointer user_data)
{
  AppData *gApp = (AppData *) user_data;

  if (infVector.size() != namesVector.size())
    return;

  cairo_select_font_face(cr,
                         "Arial",
                         CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, perfFontSize);

  std::string pipeDuration;
  std::string FPS;
  if (hasPerf.freq == true)
    FPS = std::to_string(gApp->FPS).substr(0, 5) + " FPS";

  if (hasPerf.temp == true) {
    pipeDuration = std::to_string(1000/gApp->FPS).substr(0, 5) + " ms";
    if ((hasPerf.temp == true) && (hasPerf.freq == true))
      pipeDuration.append(" / ");
  }
  outlineText(cr, 14, 18, ("Pipeline: " + pipeDuration + FPS), perfColor);

  std::string inference;
  std::string IPS;
  for (int i = 0; i < namesVector.size(); i++) {
    if (hasPerf.freq == true)
      IPS = std::to_string(1000000.0/infVector.at(i)).substr(0, 5) + " IPS";

    if (hasPerf.temp == true) {
      inference = std::to_string(infVector.at(i)/1000.0).substr(0, 5) + " ms";
      if ((hasPerf.temp == true) && (hasPerf.freq == true))
        inference.append(" / ");
    }
    outlineText(cr, 14, 38 + 20*i, ("Inference for " + namesVector.at(i) + " : " + inference + IPS), perfColor);
  }
}


/**
 * @brief Get tensor_filter name of all pipelines.
 * 
 * @param name: name of tensor_filter element.
 */
void GstPipelineImx::addFilterName(std::string name) {
  gApp.filterNames.push_back(name);
  namesVector.push_back(name);
}