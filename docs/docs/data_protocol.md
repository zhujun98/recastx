## Stream Data Processing

RECASTX was designed to process data streams in near real time. There are two scenarios: receiving data from
the data acquisition (DAQ) interface or streaming data from files. 

## Data Protocol

The data processing pipeline was originally designed to handle a challenging and unusual scenario, in which the
data stream produced by the DAQ is unordered. Therefore, the pipeline relies on the "frame" parameter, which should
be sent together with the image data, to sort the incoming data. The frame index is also used to determine when all the images
in a scan have arrived and ready for reconstructing a tomogram. Currently, the pipeline cannot handle incomplete scan (e.g. one 
or more images are dropped for whatever reason). The unprocessed data will simply be dropped when all the images 
in a later scan has arrived.

The pipeline distinguishes three types of image data (dark, flat and projection) by using another parameter "scan_index"
arriving together with the image data. The shape of the image is also required. Please check `StdDaqClient::parseData` for 
more details.

If you are also using ZeroMQ, you can easily subclass `ZmqDaqClient` to make your own DAQ client class. Otherwise, 
you can subclass `DaqClientInterface` if you are using a different messaging library for data streaming.

