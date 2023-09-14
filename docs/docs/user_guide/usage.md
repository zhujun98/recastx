![architecture](../media/recastx-architecture.png)

### At TOMCAT

#### Step 1: Start the GUI

Log in (no ssh) onto the graphics workstation `x02da-gws-3` and open a terminal
```sh
conda activate recastx-gui
recastx-gui --recon-host x02da-gpu-1
```

Or on the Ra cluster, you can only run the OpenGL GUI inside a [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) client by
```sh
vglrun recastx-gui --recon-host x02da-gpu-1
```
**Note**: there is still a problem of linking the OpenGL GUI on Ra.

Or on a local PC
```sh
ssh -L 9971:localhost:9971 x02da-gpu-1
```

#### Step 2: Start the reconstruction server

```sh
conda activate recastx-recon

# Receiving the data stream and running the GUI both locally
recastx-recon  --rows 800 --cols 384 --angles 400

# Receiving the data stream from a DAQ node
recastx-recon --daq-host xbl-daq-36 --rows 800 --cols 384 --angles 400
```

For more information, type
```sh
recastx-recon -h
```

#### Step 3: Stream the data

**Option1**: Streaming data from files on the GPU node

Install [foamstream](https://github.com/zhujun98/foamstream.git), and stream real
experimental data, for example, by
```sh
foamstream-tomcat --datafile pet1 --ordered

# pet1: recastx-recon --rows 800 --cols 384 --angles 400 --threads 32
# h1: recastx-recon --rows 2016 --cols 288 --angles 500 --threads 32

```
or stream fake data, for example, by
```sh
foamstream-tomcat --rows 800 --cols 384 --projections 10000
```

**Option2**: On the DAQ node

Start data acquisition with GigaFRoST camera.

### At other facilities

TBD: a data adaptor should be needed