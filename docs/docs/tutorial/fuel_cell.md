## Introduction

In this hands-on tutorial, we will take a look at how we can use RECASTX to explore
a 4D dataset from a dynamic tomography scan (tomoscopy).

## Prerequisites

This tutorial assumes you have already followed our [installation guide](../installation.md)
to install the reconstruction server and the GUI client on a server machine and a client
machine, respectively. You will also need to install
[foamstream](https://github.com/zhujun98/foamstream.git) on the server machine in order
to stream the data from files.

We are going to use the *Foam* dataset offered by
[TomoBank](https://tomobank.readthedocs.io/en/latest/#). Go to the homepage, and then navigate
to `Datasets/Dynamic/Fuel cell data` and download the file `fuelcell_i3.h5`. 
You can also find the description of the dataset there.

## Running

### Streaming the data

Open a terminal and run:
```bash
foamstream-tomo --datafile <Your/folder/fuelcell_i3.h5>
```

### Starting the reconstruction server

Open another terminal and run:
```bash
recastx-recon --rows 1100 --cols 1400 --angles 301
```

You can find the shapes of the DARK, FLAT and PROJECTION data from the output of foamstream.

### Starting the GUI client

Open a terminal and run:
```bash
recastx-gui
```

Make sure the `SCAN MODE` is set to `Discrete` and click the `Process` button.

<figure markdown>
  ![Fuelcell slices](../recastx-docs-supplement/fuelcell_slices.gif){ width="480" }
  <figcaption>Three high-resolution slices</figcaption>
</figure>

!!! note
    Since the GPU cluster that we used for recording this tutorial is not performant,
    the raw data were downsampled by a factor of 2 (one can configure it in the GUI)
    and thus the quality of the reconstructed slice degrades slightly. We also turned off
    the volume reconstruction in order to save the network bandwidth for visualization.
    See [Performance consideration](../performance_consideration.md)
    for more details. Therefore, there is no preview when moving the slice.

<figure markdown>
  ![Fuelcell volume](../recastx-docs-supplement/fuelcell_volume.jpg){ width="480" }
  <figcaption>"Low-resolution" volume</figcaption>
</figure>
