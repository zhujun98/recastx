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


def stream_data_file(filepath, socket,  scan_index, n):
    queue = Queue()
    thread = Thread(target=send, args=(socket, queue))
    thread.start()

    with h5py.File(filepath, "r") as fp:
        if scan_index == 0:
            ds = fp["/exchange/data_dark"]
        elif scan_index == 1:
            ds = fp["/exchange/data_white"]
        elif scan_index == 2:
            ds = fp["/exchange/data"]
        else:
            raise ValueError(f"Unsupported scan_index: {scan_index}")

        shape = ds.shape[1:]
        data = np.zeros(shape, dtype=np.uint16)
        n_images = ds.shape[0]
        for i in range(n):
            idx = i % n_images
            meta = gen_meta(scan_index, i, shape)
            ds.read_direct(data, np.s_[idx, ...], None)
            queue.put((meta, data))
            print(f"Sent type {scan_index}, frame {i}")

    print("Number of data sent: ", i+1)
    
    queue.put((None, sentinel))
    thread.join()


def main():
    parser = argparse.ArgumentParser(description='Fake GigaFrost Data Stream')

    parser.add_argument('--port', default="5558", type=int)
    parser.add_argument('--sock', default='pub', type=str)
    parser.add_argument('--darks', default=20, type=int)
    parser.add_argument('--flats', default=20, type=int)
    parser.add_argument('--projections', default=128, type=int)
    parser.add_argument('--rows', default=1200, type=int)
    parser.add_argument('--cols', default=2016, type=int)
    parser.add_argument(
        '--datafile', 
        help="Known test data: /sls/X02DA/Data10/e16816/disk1/PET_55um_40_1/PET_55um_40_1.h5", 
        type=str)

    args = parser.parse_args()
    port = args.port
    sock_type = args.sock.upper()

    context = zmq.Context()

    if sock_type == "PUSH":
        socket = context.socket(zmq.PUSH)
    elif sock_type == "PUB":
        socket = context.socket(zmq.PUB)
    else:
        raise RuntimeError(f"Unknow sock type: {sock_type}")
    socket.bind(f"tcp://*:{port}")

    shape = (args.rows, args.cols)
    for scan_index, n in enumerate([args.darks, args.flats, args.projections]):
        if not args.datafile:
            gen_fake_data(socket, scan_index, n, shape=shape)
        else:
            stream_data_file(args.datafile, socket, scan_index, n)
 

if __name__ == "__main__":
    main()
