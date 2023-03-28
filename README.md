# RECAST-X

**REC**onstruction of **A**rbitrary **S**lices in **T**omography - e**X**treme

---



This project was developed based on a successful proof-of-principle test [1] using [RECAST3D](https://github.com/cicwi/RECAST3D.git) in 2019 at the 
[TOMCAT](https://www.psi.ch/en/sls/tomcat), [Swiss Light Source](https://www.psi.ch/en/sls).

*References*

[1] Buurlage, JW., Marone, F., Pelt, D.M. et al. Real-time reconstruction and visualisation towards dynamic feedback control during time-resolved tomography experiments at TOMCAT. Sci Rep 9, 18379 (2019). https://doi.org/10.1038/s41598-019-54647-4

## Installation

```sh
module load gcc/9.3.0
```

### Installing the reconstruction server


On the GPU node `x02da-gpu-1`

```sh
cd /afs/psi.ch/project/TOMCAT_dev/recastx
git clone --recursive <repo>

conda env create -f environment-recon.yml
conda activate recastx-recon

mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_TEST=ON 
make -j12 && make install
```

### Installing the GUI

On the graphics workstation `x02da-gws-3`

```sh
cd /afs/psi.ch/project/TOMCAT_dev/recastx
git clone --recursive <repo>

conda env create -f environment-gui.yml
conda activate recastx-gui

mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON -DBUILD_TEST=ON 
make -j12 && make install
```

## Running at TOMCAT

### Step 1: Start the GUI 

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
ssh -L 9970:localhost:9970 -L 9971:localhost:9971 x02da-gpu-1
```

### Step 2: Start the reconstruction server

```sh
conda activate recastx-recon

# Receiving the data stream and running the GUI both locally
recastx-recon --threads 32 --rows 800 --cols 384 --angles 400

# Receiving the data stream from a DAQ node
recastx-recon --daq-host xbl-daq-36 --threads 32 --rows 800 --cols 384 --angles 400
```

For more information, type
```sh
recastx-recon -h
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


## Running at other facilities

**TBD**: a data adaptor should be needed
