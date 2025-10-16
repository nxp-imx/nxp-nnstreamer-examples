# NXP NNStreamer
NXP NNStreamer is a C++17 library developed by NXP that leverages NNStreamer capabilities for machine learning applications. It aims to help developers easily integrate video processing and neural network inference pipelines on NXP hardware platforms. <br>
NXP NNStreamer library is used by all examples implemented in C++. To check if an example is using the library, you can go through the Tasks section of the [main documentation](../../README.md).

## Table of Contents
- [Quick Start](#quick-start)
- [Pipeline Architecture](#pipeline-architecture)
- [Input Sources](#input-sources)
- [Pre-processing](#pre-processing)
- [Parallelization](#parallelization)
- [ML Model Processing](#ml-model-processing)
- [Post-processing](#post-processing)
- [Running the Pipeline](#running-the-pipeline)
- [Complete Example](#complete-examples)

## <a name="quick-start"></a> Quick Start

### Basic Pipeline Creation
Every pipeline starts with creating a `GstPipelineImx` object:

```cpp
#include "common.hpp"

// Create an empty pipeline
GstPipelineImx pipeline;
```
NOTE:
* Headers can be called independelty or use common.hpp

### Simple Camera Pipeline Example
```cpp
// 1. Create pipeline
GstPipelineImx pipeline;

// 2. Add camera input
CameraOptions camOpt = {
    .cameraDevice   = "/dev/video3",
    .gstName        = "cam_src",
    .width          = 640,
    .height         = 480,
    .horizontalFlip = false,
    .format         = "YUY2",
    .framerate      = 30,
};
GstCameraImx camera(camOpt);
camera.addCameraToPipeline(pipeline);

// 3. Add display output
GstVideoPostProcess postProcess;
postProcess.display(pipeline);

// 4. Run pipeline
pipeline.parse();
pipeline.run();
```

## <a name="pipeline-architecture"></a> Pipeline Architecture

The NXP NNStreamer pipeline follows this general flow:

```
Input → Pre-processing → ML Processing → Post-processing → Output
  ↓           ↓              ↓                  ↓           ↓
Camera    Resize/Crop    Inference         Overlay/Mix   Display/Save
Video     Format Conv.   Decoding          Text/Cairo    Video File
Images    Color Space    Custom            Performance   
```

## <a name="input-sources"></a> Input Sources

### Camera Input

The `GstCameraImx` class handles camera input with hardware acceleration support.

```cpp
CameraOptions camOpt = {
    .cameraDevice   = "/dev/video3",     // Camera device path
    .gstName        = "cam_src",         // GStreamer element name
    .width          = 640,               // Frame width
    .height         = 480,               // Frame height
    .horizontalFlip = false,             // Mirror horizontally
    .format         = "YUY2",            // Pixel format
    .framerate      = 30,                // Frames per second
};

GstCameraImx camera(camOpt);
camera.addCameraToPipeline(pipeline);
```

**Default values:** format="" (auto), framerate=30

### Video File Input

The `GstVideoFileImx` class supports various video formats and codecs.

```cpp
GstVideoFileImx video("/path/to/video.mp4",  // Video file path
                      true,                  // Loop playback
                      640,                   // Resize width (optional)
                      480);                  // Resize height (optional)

video.addVideoToPipeline(pipeline);
```

**Supported formats:** MP4, MKV, WEBM
**Supported codecs:** H264, H265, VP9

### Image Slideshow Input

The `GstSlideshowImx` class creates a slideshow from sequentially named images.

```cpp
// Images must be named: image0001.jpg, image0002.jpg, etc.
GstSlideshowImx slideshow("/path/to/images/image%04d.jpg",
                          640,    // Resize width (-1 to keep original)
                          480);   // Resize height (-1 to keep original)

slideshow.addSlideshowToPipeline(pipeline);
```

**Requirements:** 
- Only JPEG images supported
- Sequential naming pattern required
- Default dimensions: -1 (original size)


## <a name="pre-processing"></a> Pre-processing

The `GstVideoImx` class provides hardware-accelerated pre-processing tools.

### Video Transformation

```cpp
GstVideoImx videoProcessor;

videoProcessor.videoTransform(
    pipeline,     // Pipeline object
    "RGB",        // Output format ("" to keep current)
    640,          // Width (-1 to keep current)
    480,          // Height (-1 to keep current)
    false,        // Horizontal flip
    false,        // Force square aspect ratio
    false         // Use CPU instead of hardware acceleration
);
```

### Hardware-Agnostic RGB Conversion

Some hardware accelerators (imxvideoconvert_g2d and imxvideoconvert_pxp) don't support direct RGB conversion. The `videoscaleToRGB` method provides a fallback solution that uses CPU-based conversion (such as RGBA to RGB) when hardware acceleration is not available for RGB color space conversion.

```cpp
GstVideoImx videoProcessor;

videoProcessor.videoscaleToRGB(
    pipeline,     // Pipeline object
    640,          // Target width
    480           // Target height
);
```

### Video Cropping

```cpp
GstVideoImx videoProcessor;

videoProcessor.videocrop(
    pipeline,     // Pipeline object
    640,          // Crop width
    480           // Crop height
);
```

## <a name="parallelization"></a> Parallelization

Create parallel processing branches for better performance:

```cpp
// Create tee element for parallelization
std::string teeName = "processing_tee";
pipeline.doInParallel(teeName);

// Branch 1: ML inference
GstQueueOptions nnQueue = {
    .queueName     = "ml_thread",
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
};
pipeline.addBranch(teeName, nnQueue);
// Add ML processing here...

// Branch 2: Video processing
GstQueueOptions videoQueue = {
    .queueName     = "video_thread", 
    .maxSizeBuffer = 2,
    .leakType      = GstQueueLeaky::downstream,
};
pipeline.addBranch(teeName, videoQueue);
// Add video processing here...
```

## <a name="ml-model-processing"></a> ML Model Processing

### TensorFlow Lite Inference

```cpp
TFliteModelInfos model("/path/to/model.tflite",  // Model path
                       "NPU",                    // Backend: CPU, GPU, NPU
                       "centeredScaled",         // Normalization type
                       2);                       // number of threads for CPU ops (optional)

model.addInferenceToPipeline(pipeline,           // Pipeline object
                            "inference_element", // Element name (optional)
                            "RGB");              // Input format (optional)
```

**Supported backends:** CPU, GPU, NPU
**Normalization options:** "none", "centered", "scaled", "centeredScaled"
***Default value:** number of threads=max available threads, element name="", format="RGB"

### Built-in Decoders

#### Image Classification
```cpp
NNDecoder decoder;
decoder.addImageLabeling(pipeline, "/path/to/labels.txt");
```

#### Object Detection
```cpp
NNDecoder decoder;

SSDMobileNetCustomOptions customOptions = {
    .boxesPath = "/path/to/boxes.txt",
};

BoundingBoxesOptions options = {
    .modelName    = ModeBoundingBoxes::mobilenetssd,
    .labelsPath   = "/path/to/labels.txt",
    .option3      = setCustomOptions(customOptions),
    .outDim       = {640, 480},                           // Display dimensions
    .inDim        = {model.getModelWidth(), model.getModelHeight()},
    .trackResult  = false,                                // Enable tracking
    .logResult    = false,                                // Enable logging
};

decoder.addBoundingBoxes(pipeline, options);
```

**Supported models:** mobilenetssd, yolov5, palm_detection

#### Image Segmentation
```cpp
NNDecoder decoder;

ImageSegmentOptions options = {
    .modelName = ModeImageSegment::tfliteDeeplab,
    .numClass  = 21,  // -1 for auto-detection
};

decoder.addImageSegment(pipeline, options);
```

**Supported models:** TFlite Deeplab, SNPE Deeplab, SNPE Depth

NOTE
* Option 3 depends on the model used, for mobilenet SSD use `SSDMobileNetCustomOptions` structure, for yolov5 use `YoloCustomOptions` structure, and for MP palm detection use `PalmDetectionCustomOptions` structure

### Custom Decoder

For custom inference result processing:

```cpp
// Custom data structure for sharing between callbacks
struct CustomDecoderData {
    std::vector<float> results;
    bool newDataAvailable = false;
    // Add your custom fields here
};

// Callback for processing inference output
void inferenceCallback(GstElement* element, GstBuffer* buffer, gpointer user_data) {
    CustomDecoderData* data = (CustomDecoderData*) userData;
    // Process inference results
    // Update data->results
    data->newDataAvailable = true;
}

// Callback for drawing overlay
void drawCallback(GstElement* overlay, cairo_t* cr, guint64 timestamp, 
                  guint64 duration, gpointer user_data) {
    CustomDecoderData* data = (CustomDecoderData*) userData;
    if (data->newDataAvailable) {
        // Draw custom overlay using Cairo
        // Use data->results for drawing
    }
}

// Setup custom decoder
std::string tensorSinkName = "custom_sink";
std::string overlayName = "custom_overlay";

pipeline.addTensorSink(tensorSinkName);

GstVideoPostProcess postProcess;
postProcess.addCairoOverlay(pipeline, overlayName);

// Parse pipeline before connecting signals
pipeline.parse();

// Connect callbacks
CustomDecoderData decoderData;
pipeline.connectToElementSignal(tensorSinkName, inferenceCallback, 
                                "new-data", &decoderData);
pipeline.connectToElementSignal(overlayName, drawCallback, 
                                "draw", &decoderData);
```
NOTE
* Implementation of custom decoder can be found in [pose detection](./../../pose/cpp/example_pose_movenet_tflite.cpp) or [face detection](./../../face/cpp/example_face_detection_tflite.cpp) examples

## <a name="post-processing"></a> Post-processing

### Display Output

```cpp
GstVideoPostProcess postProcess;

// Display with performance metrics
postProcess.display(pipeline, 
                   PerformanceType::all,  // Performance type
                   "green");              // Text color
```

**Performance types:**
- `PerformanceType::none` - No metrics
- `PerformanceType::temporal` - Inference time, pipeline duration
- `PerformanceType::frequency` - FPS, IPS (Inferences Per Second)
- `PerformanceType::all` - All metrics

**Colors:** "white" (default), "red", "green", "blue", "black"


NOTE
* To display performances of a model, `element name` needs to be set (see [ML Model Processing](#ml-model-processing) section)

### Video Mixing

Combine multiple video streams:

```cpp
// Create compositor
std::string compositorName = "video_mixer";
GstVideoCompositorImx compositor(compositorName);

// Configure first input
compositorInputParams input1 = {
    .position     = displayPosition::center,
    .order        = 1,           // Display order (higher = on top)
    .keepRatio    = true,        // Maintain aspect ratio
    .transparency = false,       // Enable transparency support
};
compositor.addToCompositor(pipeline, input1);

// Add more inputs as needed...

// Add compositor to pipeline
int latency = 10000000; // nanoseconds
compositor.addCompositorToPipeline(pipeline, latency);
```

**Display positions:** center, left, right

### Text Overlay

```cpp
GstVideoPostProcess postProcess;

TextOverlayOptions overlayOptions = {
    .gstName    = "text_overlay",
    .fontName   = "Sans Bold",
    .fontSize   = 24,
    .color      = "white",
    .vAlignment = "top",         // Vertical alignment
    .hAlignment = "left",        // Horizontal alignment
    .text       = "Custom Text", // Static text (optional)
};

postProcess.addTextOverlay(pipeline, overlayOptions);

// For dynamic text, link to another element
pipeline.linkToTextOverlay("text_overlay");
```

**Text color:** "white" (default), "red", "green", "blue", "black"
**Vertical alignment:** "top", "center", "bottom", "baseline"
**Horizontal alignment:** "left", "center", "right"

### Save to Video

```cpp
GstVideoPostProcess postProcess;

postProcess.saveToVideo(pipeline,
                       "mp4",                    // Format: "mp4" or "mkv"
                       "/path/to/output.mp4");   // Output file path
```

## <a name="running-the-pipeline"></a>  Running the Pipeline

### Basic Execution

```cpp
// Parse the pipeline (converts to GStreamer pipeline)
char* graphPath = nullptr; // Optional graph path, can be nullptr if not using i.MX8MPlus
pipeline.parse(graphPath);

// Run the pipeline
pipeline.run();
```

### With Custom Elements

When using custom decoders or callbacks:

```cpp
// Parse first
pipeline.parse();

// Connect custom callbacks
pipeline.connectToElementSignal(elementName, callback, signal, userData);

// Then run
pipeline.run();
```

## <a name="complete-examples"></a> Complete Example

### Video Processing with Parallel Branches

```cpp
#include "common.hpp"

int main() {
    GstPipelineImx pipeline;
    
    // Add video input
    GstVideo video("/path/to/input.mp4", false, 640, 480);
    video.addVideoToPipeline(pipeline);
    
    // Create parallel processing
    std::string teeName = "parallel_tee";
    pipeline.doInParallel(teeName);
    
    // Branch 1: ML processing
    GstQueueOptions mlQueue = {
        .queueName = "ml_branch",
        .maxSizeBuffer = 2,
        .leakType = GstQueueLeaky::downstream,
    };
    pipeline.addBranch(teeName, mlQueue);
    
    // Add ML model to first branch
    TFliteModelInfos classifier("/path/to/classifier.tflite", "CPU", "centeredReduced");
    classifier.addInferenceToPipeline(pipeline, "classifier");
    
    NNDecoder decoder;
    decoder.addImageLabeling(pipeline, "/path/to/labels.txt");

    // For dynamic text
    pipeline.linkToTextOverlay("text_overlay");
    
    // Branch 2: Direct video processing  
    GstQueueOptions videoQueue = {
        .queueName = "video_branch",
        .maxSizeBuffer = 2, 
        .leakType = GstQueueLeaky::downstream,
    };
    pipeline.addBranch(teeName, videoQueue);
    
    // Mix branches and display
    GstVideoPostProcess postProcess;
    TextOverlayOptions overlayOptions = {
      .gstName    = "text_overlay",
      .fontName   = "Sans",
      .fontSize   = 24,
      .color      = "",
      .vAlignment = "baseline",
      .hAlignment = "center",
      .text       = "", // Dynamic text already linked
    };
    postProcess.addTextOverlay(pipeline, overlayOptions);
    postProcess.display(pipeline, PerformanceType::frequency);
    
    // Run
    pipeline.parse();
    pipeline.run();
    
    return 0;
}
```

## Best Practices

1. **Hardware Acceleration**: Use NPU backend for inference when available
2. **Memory Management**: Set appropriate queue buffer sizes for your use case
3. **Performance**: Use parallel processing for complex pipelines
4. **Error Handling**: Always check return values and handle errors appropriately
5. **Resource Cleanup**: Ensure proper cleanup of pipeline resources

## Troubleshooting

- **Camera not found**: Check device path and permissions
- **Model loading fails**: Verify model format and path
- **Performance issues**: Consider using hardware acceleration and parallel processing
- **Memory issues**: Adjust queue buffer sizes and leak policies
