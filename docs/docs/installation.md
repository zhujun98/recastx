A modern C++ compiler supporting C++17 is required.

```sh
module load gcc/11.3.0
```

## Installing the reconstruction server


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

## Installing the GUI

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