import numpy as np
import zmq
import json
import argparse
from queue import Queue
from threading import Thread

import h5py


sentinel = object()


def send(socket, queue):
    while True:
        meta, data = queue.get()
        if data is sentinel:
            break
        socket.send_json(meta, flags=zmq.SNDMORE)
        socket.send(data)


def gen_meta(scan_index, frame_index, shape):
    return {
        'image_attributes': {
            'scan_index': scan_index,
            'image_is_complete': True,
        },
        'frame': frame_index,
        'source': 'gigafrost',
        'shape': shape,
        'type': 'uint16'
    }


def gen_fake_data(socket, scan_index, n, *, shape):
    queue = Queue()
    thread = Thread(target=send, args=(socket, queue))
    thread.start()

    for i in range(n):
        meta = gen_meta(scan_index, i, shape)
        if scan_index == 0:
            data = np.random.randint(500, size=shape, dtype=np.uint16)
        elif scan_index == 1:
            data = 3596 + np.random.randint(500, size=shape, dtype=np.uint16)
        else:
            data = np.random.randint(4096, size=shape, dtype=np.uint16)
        queue.put((meta, data))

    print("Number of data sent: ", i+1)

    queue.put((None, sentinel))
    thread.join()


def stream_data_file(filepath, socket,  scan_index, n, *, i0=0):
    queue = Queue()
    thread = Thread(target=send, args=(socket, queue))
    thread.start()

    with h5py.File(filepath, "r") as fp:
        if scan_index == 0:
            ds = fp["/exchange/data_dark"]
            print(f"Dark data shape: {ds.shape}")
        elif scan_index == 1:
            ds = fp["/exchange/data_white"]
            print(f"Flat data shape: {ds.shape}")
        elif scan_index == 2:
            ds = fp["/exchange/data"]
            print(f"Projection data shape: {ds.shape}")
        else:
            raise ValueError(f"Unsupported scan_index: {scan_index}")

        shape = ds.shape[1:]
        data = np.zeros(shape, dtype=np.uint16)
        n_images = ds.shape[0]
        for i in range(i0, i0 + n):
            idx = i % n_images
            meta = gen_meta(scan_index, i, shape)
            ds.read_direct(data, np.s_[idx, ...], None)
            queue.put((meta, data))
            print(f"Sent type {scan_index}, frame {i}")

    print("Number of data sent: ", i+1)
    
    queue.put((None, sentinel))
    thread.join()


def parse_datafile(name: str):
    if name == "pet":
        # number of projections per scan: 400
        return "/sls/X02DA/Data10/e16816/disk1/PET_55um_40_1/PET_55um_40_1.h5"
    return name


def main():
    parser = argparse.ArgumentParser(description='Fake GigaFrost Data Stream')

    parser.add_argument('--port', default="5558", type=int)
    parser.add_argument('--sock', default='push', type=str)
    parser.add_argument('--darks', default=20, type=int)
    parser.add_argument('--flats', default=20, type=int)
    parser.add_argument('--projections', default=128, type=int)
    parser.add_argument('--p0', default=0, type=int,
                        help="Starting index of the projection images")
    parser.add_argument('--rows', default=1200, type=int,
                        help="Number of rows of the generated image")
    parser.add_argument('--cols', default=2016, type=int,
                        help="Number of columns of the generated image")
    parser.add_argument('--datafile', type=str, 
                        help="Path or code of the data file")

    args = parser.parse_args()
    port = args.port
    sock_type = args.sock.upper()

    datafile = parse_datafile(args.datafile)

    context = zmq.Context()

    if sock_type == "PUSH":
        socket = context.socket(zmq.PUSH)
    elif sock_type == "PUB":
        socket = context.socket(zmq.PUB)
    else:
        raise RuntimeError(f"Unknow sock type: {sock_type}")
    socket.bind(f"tcp://*:{port}")

    if datafile:
        print(f"Streaming data from {datafile} ...")
    for scan_index, n in enumerate([args.darks, args.flats, args.projections]):
        if not datafile:
            gen_fake_data(socket, scan_index, n, shape=(args.rows, args.cols))
        else:
            stream_data_file(datafile, socket, scan_index, n, 
                             i0=args.p0 if scan_index == 2 else 0)
 

if __name__ == "__main__":
    main()
