
## Introduction

In this hands-on tutorial, we will take a look at how we can use RECASTX to explore
a 3D dataset from a static tomography scan.

## Prerequisites

This tutorial assumes you have already followed our [installation guide](../installation.md) 
to install the reconstruction server and the GUI client on a server machine and a client 
machine, respectively. You will also need to install 
[foamstream](https://github.com/zhujun98/foamstream.git) on the server machine in order 
to stream the data from files.

We are going to use the *plastic beeds* dataset offered by 
[TomoBank](https://tomobank.readthedocs.io/en/latest/#). Open its homepage and Navigate 
to `Datasets/KBLT` and download the file `2_plastic_beeds_RGB.h5`.

## Running

### Streaming the data

Open a terminal and run:
```sh
foamstream-tomcat --datafile <path>/2_plastic_beeds_RGB.h5 --pdata tomo -pflat flat
```

### Starting the reconstruction server

Open another terminal and run:
```sh
recastx-recon --rows 400 --cols 130 --angles 200 --minx -256 --maxx 256 --miny -256 --maxy 256
```

You can find the shapes of the DARK, FLAT and PROJECTION data from the output of foamstream. 

### Starting the GUI client

Open a terminal and run:
```sh
recastx-gui
```

<figure markdown>
  ![Beads volume](../recastx-docs-supplement/beads_volume.gif){ width="480" }
  <figcaption>Low-resolution volume</figcaption>
</figure>

<figure markdown>
  ![Beads slices](../recastx-docs-supplement/beads_slices.jpg){ width="480" }
  <figcaption>Three orthogonal high-resolution slices</figcaption>
</figure>