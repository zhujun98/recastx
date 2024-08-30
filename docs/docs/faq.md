### When do I need to re-collect Dark and Flat images?

In the current design, there is no dedicated session for Dark and Flat images collection. In another word, one does not
need to click a button to start collecting Dark and Flat images and then stop it. I believe it would need extra
coordination and could be error-prone during experiments. 

To start a reconstruction, one should send Dark and Flat images prior to sending the Projection ones.
The original Dark and Flat images will be stored. That is to say, if you change the downsampling factor, you don't need 
to re-collect them. The stored Dark and Flat images will be discarded only in one of the following scenarios:

1. A Dark or Flat image was received and the reciprocal has already been calculated (i.e. at least one Projection image 
   had been received afterward). The software assumes that you want to re-collect Dark and Flat images.

2. The original image size has been changed.

### Why the throughput on my machine is below the claimed 3 GB/s?

See [Performance Consideration](./performance_consideration.md)

### How can we further improve the throughput?

Use a GPU node with a decent number (>72) of CPU threads.

### Why do we get the warning message like "Received projection with outdated chunk index: 1234, data ignored!"

As mentioned in [Data Protocol](./data_protocol.md), the data pipeline relies on the "frame" parameter to sort
the incoming projections. If the received frame is believed to be an outdated frame, it will be dropped.

