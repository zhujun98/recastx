## When do I need to re-collect Dark and Flat images?

One should send Dark and Flat images prior to sending the Projection images.
The original Dark and Flat images will be stored. That is to say, if you change the downsampling factor, you don't need to re-collect them. The stored Dark and Flat images will be discarded only in one of the following scenarios:

1. A Dark or Flat image was received and the reciprocal has already been calculated (i.e. at least one Projection image was received afterwards). The program assumes that you want to re-collect them.

2. The original image size has been changed.

## How can we further improve the throughput?

Use a GPU node with a decent number (>72) of threads.