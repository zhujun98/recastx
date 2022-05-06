# TOMCAT-LIVE

## Installation

```sh
# On Ra
module load gcc/9.3.0
conda create -n tomcat-live python==3.7.10

conda activate tomcat-live
conda install -c conda-forge cmake cppzmq eigen boost fftw libastra nlohmann_json spdlog pybind11

git clone --recursive <repo>
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../" \
         -DBUILD_PYTHON=ON 

make -j12

make install
```

## Running

To run it on the Ra cluster, currently we still need the GUI installed from conda:
```sh
conda install -c cicwi -c astra-toolbox/label/dev recast3d
```

**Step 1**: Start the GUI in a [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) client. 
```
vglrun recast3d
```
**Step 2**: start the reconstruction server by
```
conda activate tomcat-live
tomcat-live-server
```
**Step 3**: ssh to the same node in another terminal and push the test data 
to the reconstruction server by
```
python examples/fake_stream.py
```