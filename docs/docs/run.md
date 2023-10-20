## Step 1: Starting the reconstruction server

```bash
conda activate recastx-recon
```

For a local data source:
```bash
recastx-recon  --rows 800 --cols 384 --angles 400
```

For a remote data source (e.g. DAQ node):
```bash
recastx-recon --rows 800 --cols 384 --angles 400 --daq-address <hostname:port> 
```

For more information, type
```bash
recastx-recon -h
```

## Step 2: Starting the GUI

```bash
conda activate recastx-gui
```

You can specify the reconstruction server
```bash
recastx-gui --server <hostname:port>
```

or make use of local port forwarding
```bash
ssh -L <port>:localhost:<port> <hostname>
recastx-gui
```

You can also start the GUI on a node with [NoMachine](https://www.psi.ch/en/photon-science-data-services/remote-interactive-access
) installed
```bash
vglrun recastx-gui --server <hostname:port>
```

## Step 3: Streaming the data

### Option 1: streaming data from an area detector

Contact the corresponding specialists at the facility.

### Option 2: streaming data from files

We recommend using [foamstream](https://github.com/zhujun98/foamstream.git).
```bash
pip install foamstream

foamstream-tomo --datafile <Your/HDF5 file/path>
```

Please feel free to use your own file streamer as long as the [data protocol](data_protocol.md) is compatible.
