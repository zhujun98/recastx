import numpy as np
np.set_printoptions(precision=8)

from filter_expected import apply_filter


def test_reconstructor_push_data():
    shape = (4, 5)  # (rows, cols)

    darks = np.zeros((4,) + shape, dtype=np.uint16)

    flats = np.ones((6,) + shape, dtype=np.uint16)

    img_base = np.array([
        [2, 5, 3, 7, 1],
        [4, 6, 2, 9, 5],
        [1, 3, 7, 5, 8],
        [6, 8, 8, 7, 3]
    ], dtype=np.uint16)
    projections = np.stack((img_base, img_base + 1), axis=0)

    dark_avg = np.mean(darks, axis=0, dtype=np.float32)
    flat_avg = np.mean(flats, axis=0, dtype=np.float32)

    corrected = -np.log((projections - dark_avg) / (flat_avg - dark_avg))
    filtered = apply_filter(corrected, "shepp-logan")
    print(filtered)
    sino = np.moveaxis(filtered, 0, 1)
    print(sino)

if __name__ == "__main__":
    test_reconstructor_push_data()
