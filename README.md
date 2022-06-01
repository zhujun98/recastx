# TOMCAT-LIVE

## Installation

```sh
module load gcc/9.3.0
conda create -n tomcat-live python==3.7.10

conda activate tomcat-live
conda install -c conda-forge cmake cppzmq eigen boost fftw libastra tbb-devel nlohmann_json spdlog pybind11

git clone --recursive <repo>

# On the GPU node x02da-gpu-1
# Build and install the reconstruction server, the dummy consumer as well as
# the Python bindings 
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_PYTHON=ON -DBUILD_MONITOR=ON 
make -j12 && make install

# On the graphics workstation x02da-gws-3
mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON 
make -j12 && make install
```

## Running

**Step 1**: Start the GUI 

```sh
conda activate tomcat-live
tomcat-live-gui
```

On the Ra cluster, you can only run the OpenGL GUI inside a [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) client by
```sh
vglrun tomcat-live-gui
```
**Note**: there is still a problem of linking the OpenGL GUI on Ra.

**Step 2**: start the reconstruction server

```
conda activate tomcat-live
# with local data stream
tomcat-live-server --data-port 5558 --data-socket pull --gui-server x02da-gws-3 --gui-port 5555 --filter-cores 20
# with data stream from the DAQ node
# tomcat-live-server --data-host xbl-daq-36 --data-port 9610 --data-socket sub --gui-server x02da-gws-3 --gui-port 5555 --filter-cores 20
```

**Step 3**: stream the data

From the same node as the reconstruction server:

```
python examples/fake_stream.py
```

or from the DAQ node:

```
ssh x02da-gpu-1
ssh xbl-daq-36
sudo -i -u dbe

# streaming the fake data
# cd ~/git/gf_repstream
# conda activate repstream
# python -m gf_repstream.test.fake_stream -f ./gf_repstream/test/test_data -i 10000 -a tcp://*:9610 -m pub

# connecting to the GigaFrost backend
sudo systemctl {restart/status/start/stop} repstream-gf.service 
# log
#sudo journalctl -u repstream-gf.service -f
```
