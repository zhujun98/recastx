import numpy as np
np.set_printoptions(precision=8)


def test_reconstructor_push_data():
    shape = (200, 120)

    darks = np.zeros((4,) + shape, dtype=np.uint16)
    for i in range(darks.shape[0]):
        darks[i] = i

    flats = np.ones((6,) + shape, dtype=np.uint16)
    for i in range(flats.shape[0]):
        flats[i] = i + 10

    projections = 10 * np.ones((32,) + shape, dtype=np.uint16) + 10
    for i in range(projections.shape[0]):
        projections[i] = 10 * i + 10

    dark_avg = np.mean(darks, axis=0, dtype=np.float32)
    flat_avg = np.mean(flats, axis=0, dtype=np.float32)

    corrected = -np.log((projections - dark_avg) / (flat_avg - dark_avg))
    print(corrected.flatten()[:2])
    print(corrected.flatten()[119:121])
    print(corrected.flatten()[-121:-119])
    print(corrected.flatten()[-2:])

    sino = np.moveaxis(corrected, 0, 1)
    print(sino.flatten()[:2])
    print(sino.flatten()[119:121])
    print(sino.flatten()[-121:-119])
    print(sino.flatten()[-2:])

# shape = (3, 4, 3)

# darks = np.array([
#     4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8, 
#     1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2, 
#     2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0
# ], dtype=np.uint16)

# flats = np.array([
#     1, 9, 5, 1, 7, 9, 0, 6, 7, 1, 5, 6, 
#     2, 4, 8, 1, 3, 9, 5, 6, 1, 1, 1, 7, 
#     9, 9, 4, 1, 6, 8, 6, 9, 2, 4, 9, 4
# ], dtype=np.uint16)

# dark_avg = np.mean(darks.reshape(shape), axis=0, dtype=np.float32)
# flat_avg = np.mean(flats.reshape(shape), axis=0, dtype=np.float32)

# print(dark_avg)
# print(1. / (flat_avg - dark_avg))

if __name__ == "__main__":
    test_reconstructor_push_data()
