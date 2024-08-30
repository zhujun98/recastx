## Stream Data Processing

RECASTX was designed to process data streams in near real time. There are two scenarios: receiving data from
the data acquisition (DAQ) interface or streaming data from files. 

## Data Protocol

The data processing pipeline was originally designed to handle a challenging and unusual scenario, in which the
data stream produced by the DAQ is unordered. Therefore, the pipeline relies on the "frame" parameter, which should
be included in the metadata and sent along with the projections, to sort the incoming data. The frame index is also used to determine when all the projections
in a scan have arrived and are ready for a reconstruction. For example, if the number of projections per scan is 500,
the reconstruction will start when all the projections with frame number 0 - 499 (500 - 999, 1000 - 1499, etc.) have been received. 
Currently, the pipeline cannot handle incomplete scan (e.g. one or more projections are lost for whatever reason). 
The unprocessed projections will simply be discarded after all the projections in a later scan have arrived.

The pipeline distinguishes three types of image data (dark, flat and projection) by using another parameter "scan_index" 
(0 for Dark, 1 for Flat and 2 for Projection) which should also be included in the metadata. The shape of the projection is also 
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

