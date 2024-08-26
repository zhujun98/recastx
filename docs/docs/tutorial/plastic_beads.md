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
[TomoBank](https://tomobank.readthedocs.io/en/latest/#). Go to the homepage, and then navigate 
to `Datasets/KBLT` and download the file `2_plastic_beeds_RGB.h5`. 
You can also find the description of the dataset there.

## Running

### Streaming the data

Open a terminal and run:
```bash
foamstream-tomo --datafile <Your/folder/2_plastic_beeds_RGB.h5> --pdata tomo --pflat flat
```

### Starting the reconstruction server

Open another terminal and run:
```bash
recastx-recon
```

You can find the shapes of the DARK, FLAT and PROJECTION data from the output of foamstream.

!!! note
    The `volume-size`, which defines the resolution of the "low-resolution" 
    volume, is set to 512 in this case. As a result, the resolution of the 
    reconstructed volume is indeed high. It should be noted that it thus takes
    longer time to perform the reconstruction and send the reconstructed volume
    from the server to the client. Therefore, the default value of `volume-size`
    is set to 128 and is recommended to use in [dynamic tomography](./foam.md).

### Starting the GUI client

Open a terminal and run:
```bash
recastx-gui
```

Set the following parameters in the GUI:

- Set `Column Count`, `Row Count` and `Angle Count` to 130, 400 and 200, respectively.
- Tick `360 degree` radio button.
- Set `Volume Size` to 512.
- Set `X range` and `Y range` to [-256, 256].

Make sure the `SCAN MODE` is set to `Discrete` and click the `Process` button.

<figure markdown>
  ![Beads slices](../recastx-docs-supplement/kblt_beads_slices.png){ width="480" }
  <figcaption>Three orthogonal high-resolution slices</figcaption>
</figure>

<figure markdown>
  ![Beads volume](../recastx-docs-supplement/kblt_beads_volume.gif){ width="480" }
  <figcaption>"Low-resolution" volume</figcaption>
</figure>