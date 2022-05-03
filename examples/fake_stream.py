import numpy as np
import zmq
import json
import argparse



def main():
    parser = argparse.ArgumentParser(description='Fake GigaFrost Data Stream')

    parser.add_argument('--port', default="9912", type=int)
    parser.add_argument('--sock', default='push', type=str)
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
    zero_data = np.zeros(shape, dtype=np.uint16)
    one_data = np.ones(shape, dtype=np.uint16)
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
                data = zero_data
            elif scan_index == 1:
                data = one_data
            else:
                data = 2 * one_data

            socket.send(data, flags=0)
            
            print(f"Sent type {scan_index}, frame {i}")


if __name__ == "__main__":
    main()
