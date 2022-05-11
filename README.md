# TOMCAT-LIVE

## Installation

```sh
# On Ra
module load gcc/9.3.0
conda create -n tomcat-live python==3.7.10

conda activate tomcat-live
conda install -c conda-forge cmake cppzmq eigen boost fftw libastra nlohmann_json spdlog pybind11
conda install -c cicwi -c astra-toolbox/label/dev recast3d

git clone --recursive <repo>
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_PYTHON=ON 

make -j12

make install
```

## Running

**Step 1**: Start the GUI 

```sh
conda activate tomcat-live
recast3d
```

On the Ra cluster, you can only run the OpenGL GUI inside a [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) client by
```sh
vglrun recast3d
```
**Step 2**: start the reconstruction server

```
conda activate tomcat-live
# with local data stream
tomcat-live-server --filter-cores 20
# with data stream from the DAQ node
# tomcat-live-server --host xbl-daq-36 --port 9610 --filter-cores 20
```

**Step 3**: stream the data

```
python examples/fake_stream.py
```
or
```
ssh x02da-gpu-1
ssh xbl-daq-36
sudo -i -u dbe

cd ~/git/gf_repstream
conda activate repstream
python -m gf_repstream.test.fake_stream -f ./gf_repstream/test/test_data -i 10000 -a tcp://*:9610 -m pub
```
