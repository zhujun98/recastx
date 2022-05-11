import numpy as np
import zmq
import json
import argparse


def main():
    parser = argparse.ArgumentParser(description='Fake GigaFrost Data Stream')

    parser.add_argument('--port', default="5558", type=int)
    parser.add_argument('--sock', default='pub', type=str)
    parser.add_argument('--darks', default=10, type=int)
    parser.add_argument('--flats', default=10, type=int)
    parser.add_argument('--projections', default=128, type=int)
    parser.add_argument('--rows', default=1200, type=int)
    parser.add_argument('--cols', default=2016, type=int)

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
        for i in range(n):
            meta = {
                'image_attributes': {
                    'scan_index': scan_index,
                    'image_is_complete': True,
                },
                'frame': i,
                'source': 'gigafrost',
                'shape': shape,
                'type': 'uint16'
            }
            socket.send_json(meta, flags=zmq.SNDMORE)
            if scan_index == 0:
                data = np.random.randint(500, size=shape, dtype=np.uint16)
            elif scan_index == 1:
                data = 3596 + np.random.randint(500, size=shape, dtype=np.uint16)
            else:
                data = np.random.randint(4096, size=shape, dtype=np.uint16)

            socket.send(data, flags=0)
            
            print(f"Sent type {scan_index}, frame {i}")


if __name__ == "__main__":
    main()
