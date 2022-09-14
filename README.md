# TOMCAT-LIVE

## Installation

```sh
module load gcc/9.3.0
conda create -n tomcat-live python==3.7.10

conda activate tomcat-live
conda install -c conda-forge cmake cppzmq eigen xtensor boost fftw libastra tbb-devel nlohmann_json spdlog

cd /afs/psi.ch/project/TOMCAT_dev/tomcat-live
git clone --recursive <repo>

# On the GPU node `x02da-gpu-1`
# Build and install the reconstruction server, the dummy consumer as well as
# the Python bindings 
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_TEST=ON 
make -j12 && make install

# On the graphics workstation `x02da-gws-3`
mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON -DBUILD_TEST=ON 
make -j12 && make install
```

## Running

### Step 1: Start the GUI 

Log in (no ssh) onto the graphics workstation `x02da-gws-3` and open a terminal
```sh
conda activate tomcat-live
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
conda activate tomcat-live

# Receiving the data stream and running the GUI both locally
tomcat-live-server --threads 32 --rows 800 --cols 384 --group-size 400

# Receiving the data stream from a DAQ node
tomcat-live-server --data-host xbl-daq-36 --threads 32 --rows 800 --cols 384 --group-size 400
```

For more information, type
```sh
tomcat-live-server -h
```

### Step 3: Stream the data

**Option1**: From the GPU node
```sh
cd /afs/psi.ch/project/TOMCAT_dev/tomcat-live
python producer/fake_stream.py --datafile pet1 --ordered
```

**Option2**: From the DAQ node:
