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

module load gcc/12.1.0  # optional
mkdir build-recon && cd build-recon
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DCMAKE_CUDA_COMPILER=<path>
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

mkdir build-gui && cd build-gui
cmake .. -DCMAKE_PREFIX_PATH=${CONDA_PREFIX:-"$(dirname $(which conda))/../"} \
         -DBUILD_GUI=ON
make -j12 && make install
```

- Windows

## Package managers

TBD
