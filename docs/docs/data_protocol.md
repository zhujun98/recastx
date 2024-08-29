## Stream Data Processing

RECASTX was designed to process data streams in near real time. There are two scenarios: receiving data from
the data acquisition (DAQ) interface or streaming data from files. 

## Data Protocol

The data processing pipeline was originally designed to handle a challenging and unusual scenario, in which the
data stream produced by the DAQ is unordered. Therefore, the pipeline relies on the "frame" parameter, which should
be sent together with the image data, to sort the incoming data. The frame index is also used to determine when all the projections
in a scan have arrived and ready for reconstructing a tomogram. For example, if the number of projections per scan is 500,
the reconstruction will start when all the projections with frame number 0 to 499 have been received. 
Currently, the pipeline cannot handle incomplete scan (e.g. one or more images are dropped for whatever reason). 
The unprocessed data will simply be dropped when all the images in a later scan has arrived.

The pipeline distinguishes three types of image data (dark, flat and projection) by using another parameter "scan_index" 
(0 for dark, 1 for flat and 2 for projection) arriving together with the image data. The shape of the image is also 
required. Please check `StdDaqClient::parseData` for more details. To summarize, the default (current) DAQ implementation 
accepts a ZeroMQ multipart message for each frame. The first part is the metadata in a JSON format like

```json
{
  "frame": 100,
  "image_attributes": {
    "scan_index": 0
  },
  "shape": [512, 512]
}
```

, any additional fields will be ignored. The second part is the bytes of the corresponding image data.

If you are also using ZeroMQ, you can easily subclass `ZmqDaqClient` to make your own DAQ client class. Otherwise, 
you can subclass `DaqClientInterface` if you are using a different messaging library for data streaming.

