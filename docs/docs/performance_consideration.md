## Reconstruction

Currently, the bottleneck of the data processing pipeline is the preprocessing part, which 
is still implemented in CPU. The total number of threads of the current benchmark machine is 
only 32. We expect a significant performance boost on a GPU node with a decent number of threads.

## Visualization

The data rate in RECASTX is really high not only in terms of the raw 
projection images but also in terms of the reconstructed images and volumes.
In the default setup, the size of the reconstructed volume is 128 x 128 x 128 pixels,
which amounts to 8 MB. Suppose that you would like it to update at 5 Hz, the
data rate will be 40 MB/s. It should be noted that the high-resolution slices
have not been included. Even though, you may laugh off it because you already
have a 10 Gbps network connection between your client and server machines 
in your beamline or lab. *However, this could be a problem if you are going to 
conduct a remote experiment and connect your GUI client to the reconstruct server 
via SSH.*

Even with a 10 Gbps network connection between the client and the sever, the network
bandwidth could be a bottleneck if you want to increase the resolution of the
reconstructed volume. A high-resolution volume of 1024 x 1024 x 1024 pixels amounts to 
4 GB and would be difficult to update at just 1Hz (unless you compress data) 
if you don't have a 100 Gbps network connection between the client and the server.
*Therefore, you should not rule out the possibility of running the GUI client
and the reconstruction server on the same machine if it is powerful enough.*