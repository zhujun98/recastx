# TOMCAT-LIVE

## Installation

```sh
module load gcc/9.3.0
```

### Installing the reconstruction server


On the GPU node `x02da-gpu-1`

```sh
cd /afs/psi.ch/project/TOMCAT_dev/tomcat-live
git clone --recursive <repo>

conda env create -f environment-recon.yml
conda activate tomcat-live-recon

mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_TEST=ON 
make -j12 && make install
```

### Installing the GUI

On the graphics workstation `x02da-gws-3`

```sh
cd /afs/psi.ch/project/TOMCAT_dev/tomcat-live
git clone --recursive <repo>

conda env create -f environment-gui.yml
conda activate tomcat-live-gui

mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON -DBUILD_TEST=ON 
make -j12 && make install
```

## Running

### Step 1: Start the GUI 

Log in (no ssh) onto the graphics workstation `x02da-gws-3` and open a terminal
```sh
conda activate tomcat-live-gui
tomcat-live-gui --recon-host x02da-gpu-1
```

Or on the Ra cluster, you can only run the OpenGL GUI inside a [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) client by
```sh
vglrun tomcat-live-gui --recon-host x02da-gpu-1
```
**Note**: there is still a problem of linking the OpenGL GUI on Ra.

Or on a local PC
```sh
ssh -L 9970:localhost:9970 -L 9971:localhost:9971 x02da-gpu-1
```

### Step 2: Start the reconstruction server

```sh
conda activate tomcat-live-recon

# Receiving the data stream and running the GUI both locally
tomcat-live-recon --threads 32 --rows 800 --cols 384 --angles 400

# Receiving the data stream from a DAQ node
tomcat-live-recon --daq-host xbl-daq-36 --threads 32 --rows 800 --cols 384 --angles 400
```

For more information, type
```sh
tomcat-live-recon -h
```

### Step 3: Stream the data

**Option1**: Streaming data from files on the GPU node

Install [foamstream](https://github.com/zhujun98/foamstream.git), and stream real 
experimental data, for example, by
```sh
foamstream-tomcat --datafile pet1 --ordered
```
or stream fake data, for example, by
```sh
foamstream-tomcat --rows 800 --cols 384 --projections 10000
```

**Option2**: On the DAQ node:
