A modern C++ compiler supporting C++17 is required.

## From source

We recommend using the [Anaconda](https://www.anaconda.com/download) environment.

### Installing the reconstruction server

On a GPU node (Linux only),

```bash
git clone --recursive https://github.com/<repo>/recastx.git
cd recastx

conda env create -f environment-recon.yml
conda activate recastx-recon

module load gcc/11.3.0  # optional
mkdir build-recon && cd build-recon
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"}
make -j12 && make install
```

### Installing the GUI client

#### Installing prerequisites (optional)

- Linux / MaxOS

  *X11*

- Windows

#### Installing the GUI client

- Linux / MacOS

```bash
git clone --recursive https://github.com/<repo>/recastx.git
cd recastx

conda env create -f environment-gui.yml
conda activate recastx-gui

module load gcc/11.3.0  # optional
mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON
make -j12 && make install
```

- Windows

```sh
git clone --recursive https://github.com/<repo>/recastx.git
cd recastx

conda env create -f environment-gui.yml
conda activate recastx-gui

mkdir build-gui
cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=<YOUR_CONDA_INSTALL_PATH>/envs/recastx-gui/ -DBUILD_GUI=ON
msbuild recastx.sln /property:Configuration=Release
```

Note that you also have to copy the libraries manually from your Conda environment (Usually located in <YOUR_CONDA_INSTALL_PATH>/envs/recastx-gui/Library/Bin)


## Package managers

TBD

## Deployment

### At TOMCAT

- GPU node: `x02da-gpu-1`
- Graphics workstation: `x02da-gws-3`
- DAQ node: `xbl-daq-36`
