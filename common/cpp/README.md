# NXP NNStreamer C++ libraries
NXP developped a C++17 library to introduce NNStreamer potential in ML related applications.<br> Examples demonstrate how to use NNStreamer with NXP specific hardware optimizations for machine learning vision pipeline.<br>

# How to implement a pipeline
An empty pipeline object is first created using `GstPipelineImx` class : <br>
```cpp
// Create pipeline object
GstPipelineImx pipeline;
```
This object is used by various classes to add elements to the pipeline :
- [Input sources](#first-title)
- [Pre-processing tools](#second-title)
- [Parallelization of process](#third-title)
- [ML model processing tools](#fourth-title)
- [Post-processing tools](#fifth-title)
- [Performances display](#sixth-title)
- [Run pipeline](#seventh-title)

# <a name="first-title"></a> Input sources

## Camera
Camera is handled by `GstCamera` class. Camera is first initialized using the parameterized constructor with the following argument options : device name, GStreamer element name, dimension, mirror option, format, and framerate.<br> The method `addCameraToPipeline` is then used to add GStreamer camera element to the pipeline, by passing the pipeline object in argument.
```cpp
// Add camera to pipeline
GstCamera camera("/dev/video3",         // Camera device used
                 "cam_src",             // GStreamer element name
                 640,                   // Resized width
                 480,                   // Resized weight
                 false,                 // If horizontal flip
                 "YUY2",                // Format
                 30);                   // Framerate
camera.addCameraToPipeline(pipeline);   // GstPipelineImx object
```
NOTES:
* GstCamera parameterized constructor has default values for format (""), and framerate (30).

## Video
Video is handled by `GstVideo` class. Like `GstCamera` class, video is first initialized using the parameterized constructor with the following argument option : file path.<br> The method `addVideoToPipeline` is then used to add GStreamer video element to the pipeline, by passing the pipeline object in argument.
```cpp
// Add video to pipeline
GstVideo PowerJump("/path/to/video/");  // Video file path
PowerJump.addVideoToPipeline(pipeline); // GstPipelineImx object
```
NOTES:
* Video formats can only be MKV, WEBM, and MP4.

## Slideshow
slideshow is handled by `GstSlideshow` class. Like the previous classes, slideshow is first initialized using the parameterized constructor with the following argument options : images path, and dimensions (optional since images can be of different sizes).<br> The images path can't be a folder. For instance, images' name need to follow a specific syntax similar to `images0001.jpg, images0002.jpg, ...`, so the path called is `/path/to/images/images%04d.jpg`.<br> The method `addSlideshowToPipeline` is then used to add GStreamer multifilesrc element to the pipeline, by passing the pipeline object in argument.
```cpp
// Add slideshow to pipeline
GstSlideshow slideshow("/path/to/images/images%04d.jpg" // Images path
                       640,                             // Resized width
                       480);                            // Resized height
slideshow.addSlideshowToPipeline(pipeline);             // GstPipelineImx object
```
NOTES:
* Slideshow can only be composed of jpg images.
* Slideshow parameterized constructor has width and height set to -1 by default to keep original images size.


# <a name="second-title"></a> Pre-processing tools
`GstVideoImx` class have pre-processing tools which use the hardware specific accelerators to accelerate video mixing, video formatting, and video cropping.

## Video resizing and format change
`videoTransform` can be used to change video format, flip the image horizontally, and resize the GStreamer video stream :
```cpp
GstPipelineImx pipeline;
GstVideoImx gstvideoimx{};
gstvideoimx.videoTransform(pipeline,  // Apply changes to this pipeline
                           "RGB",     // New format
                           640,       // Resized width
                           480,       // Resized height
                           false,     // If horizontal flip
                           false,     // Aspect ratio set to 1/1 (square shape output image)
                           true);     // Deactivate hardware acceleration to use CPU instead

```
NOTES:
* videoTransform method has aspect ratio and useCPU set to false by default.
* To keep the same format, set argument to "".
* To keep the same dimension, set resized width and height to -1.

## Video resizing to RGB
imxvideoconvert_g2d and imxvideoconvert_pxp don't support RGB, so a specific method is used to convert a GStreamer video stream to RGB format.<br>
`videoscaleToRGB` can be used to change video format to RGB, and resize the GStreamer video stream :
```cpp
GstPipelineImx pipeline;
GstVideoImx gstvideoimx{};
gstvideoimx.videoscaleToRGB(pipeline,  // Apply changes to this pipeline
                            640,       // Resized width
                            480);      // Resized height
```

## Video cropping
`videocrop` can be used to crop a GStreamer video stream :
```cpp
GstPipelineImx pipeline;
GstVideoImx gstvideoimx{};
gstvideoimx.videocrop(pipeline,  // Apply changes to this pipeline
                      640,       // Cropped width
                      480,       // Cropped height
                      -1,        // Pixels to crop at top (-1 to auto-crop)
                      -1,        // Pixels to crop at bottom (-1 to auto-crop)
                      -1,        // Pixels to crop at left (-1 to auto-crop)
                      -1);       // Pixels to crop at right (-1 to auto-crop)
```
NOTES:
* Top, bottom, left, and right are set to 0 by default.

## Video mixing
`videoCompositor` can be used to mix two GStreamer video streams together.<br>
This method comes with `linkToVideoCompositor` method of `GstPipelineImx`, which link element the second GStreamer video stream to the video compositor :
```cpp
// First call linkToVideoCompositor to link one source to the video compositor
GstPipelineImx pipeline;
pipeline.linkToVideoCompositor("mix");
[...]
// Video compositor is called after the second source
GstVideoImx gstvideoimx{};
gstvideoimx.videoCompositor(pipeline,                   // Apply changes to this pipeline
                            "mix",                      // GStreamer element name
                            150000000,                  // Latency in ns
                            displayPosition::split);    // Position of the videos to mix
```
NOTES:
* Latency is an optional parameter set to 0 by default.
* Position is set to displayPosition::centered by default, so videos are mixed.

# <a name="third-title"></a> Parallelization of process
Parallelization is possible using `GstPipelineImx` method `doInParallel` to add a tee element, then `addBranch` method is used to create thread : <br>
```cpp
GstpipelineImx pipeline;

// Add a tee element for parallelization of tasks
std::string teeName = "t";
pipeline.doInParallel(teeName);

// Add a branch for inference and model post processing
GstQueueOptions nnQueue = {
  .queueName     = "thread-nn",
  .maxSizeBuffer = 2,
  .leakType      = GstQueueLeaky::downstream, // Where the queue leaks, if at all
};
pipeline.addBranch(teeName, nnQueue);
[...]

// Add a branch for video post processing
GstQueueOptions imgQueue = {
  .queueName     = "thread-img",
  .maxSizeBuffer = 2,
  .leakType      = GstQueueLeaky::downstream, // Where the queue leaks, if at all
};
pipeline.addBranch(teeName, imgQueue);
[...]
```

# <a name="fourth-title"></a> ML model processing tools
`TFliteModelInfos` class is used to get informations from a TFlite model to run inference. Inference depends on the backend used (CPU, GPU, or NPU).<br> `addInferenceToPipeline` method is called to add model inference to the pipeline :
```cpp
GstpipelineImx pipeline;
TFliteModelInfos classification("/path/to/model",       // Model path
                                "CPU",                  // Backend used
                                "centeredReduced");     // Input normalization
classification.addInferenceToPipeline(pipeline,         // GstPipelineImx object
                                      "classification", // Element name (optional)
                                      "GRAY8");         // Input format (optional)
```
NOTES:
* addInferenceToPipeline method has no "name" element set by default, and format set to "RGB" by default.

## NNStreamer decoder
`NNDecoder` class uses NNStreamer output decoder to process model inference output. Three types of decoding are available :<br>
`addImageSegment` method which decode segmentation models (TFlite Deeplab, SNPE Deeplab, SNPE Depth) :
```cpp
GstpipelineImx pipeline;

// Add NNStreamer decoder
NNDecoder decoder;
ImageSegmentOptions decOptions = {
  .modelName = ModeImageSegment::tfliteDeeplab, // Model to decode
  .numClass  = -1,                              // Max number of class
}
decoder.addImageSegment(pipeline, decOptions);
```
NOTES:
* numClass is an optional parameter set to -1 by default.

`addImageLabeling` method which decode classification models :
```cpp
GstpipelineImx pipeline;

// Add NNStreamer decoder
NNDecoder decoder;
decoder.addImageLabeling(pipeline, "/path/to/labels");// Labels path
```

And finally, `addBoundingBoxes` method which decode detection models (yolov5, mobilenet SSD, MP palm detection) :
```cpp
GstpipelineImx pipeline;

// Add NNStreamer decoder
NNDecoder decoder;
SSDMobileNetCustomOptions customOptions = { 
  .boxesPath = "/path/to/boxes",                                    //Boxes path
};

BoundingBoxesOptions decOptions = {
  .modelName    = ModeBoundingBoxes::mobilenetssd,                  // Model used
  .labelsPath   = "/path/to/labels",                                // Labels path
  .option3      = setCustomOptions(customOptions),                  // Option1-dependent values
  .outDim       = {640, 480},                                       // Output dimension
  .inDim        = {model.getModelWidth(), model.getModelHeight()},  // Input dimension (model dimension)
  .trackResult  = false,                                            // Whether to track result bounding boxes or not
  .logResult    = false,                                            // Whether to log the result bounding boxes or not
};
decoder.addBoundingBoxes(pipeline, decOptions);
```
NOTES:
* Option 3 depends on the model used, for mobilenet SSD use `SSDMobileNetCustomOptions` structure, for yolov5 use `YoloCustomOptions` structure, and for MP palm detection use `PalmDetectionCustomOptions` structure.

## Custom decoder
To create a custom decoding of the inference result, the inference output need to be retrieved using a tensor_sink element that can be added with `GstPipelineImx` method `addTensorSink`, and then a custom display with a Cairo overlay can be used with `GstVideoPostProcess` method `addCairoOverlay`.<br>
Then, inference decoding custom functions and custom display need to be added using callback functions, which are linked respectively to tensor_sink and Cairo overlay element signal "new-data" and "draw" using `GstPipelineImx` method `connectToElementSignal`.<br>
A data structure need to be created for variables that need to be shared between the callback functions, or need to be retrieved outside the callback functions.<br>
For instance, the inference output data are processed in tensor_sink callback function, and shared to Cairo overlay callback function to draw or display the result using a variable in the data structure :
```cpp
// Retrieves output tensor from inference output
std::string tensorSinkName = "tensor_sink";
pipeline.addTensorSink(tensorSinkName);
[...]

// Add custom display with a Cairo overlay
std::string overlayName = "cairo";
GstVideoPostProcess postProcess;
postProcess.addCairoOverlay(pipeline, overlayName);
postProcess.display(pipeline);

// Parse pipeline to GStreamer pipeline
pipeline.parse(argc, argv);

// Connect callback functions to tensor sink and cairo overlay,
// to process inference output
DecoderData kptsData;                             // Custom data structure
pipeline.connectToElementSignal(tensorSinkName,   // tensor_sink name
                                newDataCallback,  // Callback function
                                "new-data",       // tensor_sink signal
                                &kptsData);       // Custom data structure
pipeline.connectToElementSignal(overlayName,      // cairooverlay name
                                drawCallback,     // Callback function
                                "draw",           // cairooverlay signal
                                &kptsData);       // Custom data structure

// Run GStreamer pipeline
pipeline.run();
```
NOTES:
* For more details on how to make the callback functions, look at [pose detection](./../../pose/cpp/example_pose_movenet_tflite.cpp) or [face detection](./../../face/cpp/example_face_detection_tflite.cpp) examples which use custom decoder.

# <a name="fifth-title"></a> Post-processing tools
`GstVideoPostProcess` class have post-processing tools used to display GStreamer video stream, add a text overlay, and save GStreamer video stream to video.

## Display
The method `display` is used to display the GStreamer video stream :
```cpp
GstVideoPostProcess postProcess;
postProcess.display(pipeline,     // Pipeline to display
                    false);       // If display element is synchronized or not

```
NOTES:
* Second argument is set to false by default, and third argument to true.

## Text overlay
To add a text overlay to a pipeline, use `addTextOverlay` to create the GStreamer element, and then `linkToTextOverlay` method of `GstPipelineImx` if the text to display is the output of another element :
```cpp
// Link text stream to text overlay
std::string overlayName = "overlay";
pipeline.linkToTextOverlay(overlayName);
[...]
// Add text overlay
GstVideoPostProcess postProcess;
TextOverlayOptions overlayOptions = {
  .gstName    = overlayName,    // Text overlay element name
  .fontName   = "Sans",         // Font name
  .fontSize   = 24,             // Font size
  .color      = "red",          // Text color
  .vAlignment = "left",         // Horizontal alignment of the text
  .hAlignment = "bottom",       // Vertical alignment of the text
  .text       = "",             // Text to display, set only if linkToTextOverlay is not used
};
postProcess.addTextOverlay(pipeline, overlayOptions);
```
NOTES:
* Text color can be "" (white), "red", "green", "blue", and "black".
* Vertical alignement can be "baseline", "bottom", "top", "Absolute position clamped to canvas", "center", and "Absolute position".
* Horizontal alignement can be "left", "center", "right", "Absolute position clamped to canvas", and "Absolute position".

## Save to video
`GstVideoPostProcess` allows to save GStreamer video stream to MP4, and MKV using `saveToVideo` method :
```cpp
// Save output to MKV video
postProcess.saveToVideo(pipeline,
                        "mkv",                  // save video to mkv
                        "/path/to/save/video"); // path to save the video generated
```

# <a name="sixth-title"></a> Performances display
Performances can be displayed with `enablePerfDisplay` method of `GstPipelineImx` and `display` method of `GstVideoPstProcess` :
```cpp
pipeline.enablePerfDisplay(true,  // Display temporal performances (pipeline duration and inferences duration)
                           true,  // Display performances in frequency (FPS and IPS)
                           10);   // Optional parameter, setting font size (default is 15) 
postProcess.display(pipeline,
                    false);
```
NOTES:
* To display performances related to a given model (inference duration and/or IPS), it is required to give a name to the addInferenceToPipeline method that execute the model's inference using dedicating method argument. Performances will not be displayed for the addInferenceToPipeline who do not have a given name.

# <a name="seventh-title"></a>  Run pipeline
To run the pipeline, `GstPipelineImx` class method `parse` is first used to parse the pipeline to a GStreamer pipeline, and then, use `run` method :
```cpp
// Parse pipeline to GStreamer pipeline
pipeline.parse(argc, argv);

// Run GStreamer pipeline
pipeline.run();
```
Pipeline launch is divided in two parts so callback functions can be linked to elements (see custom decoder), before the pipeline is executed.
