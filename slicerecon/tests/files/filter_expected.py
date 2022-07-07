import numpy as np


def gen_filter(name, rows, cols, *, fftshift=True):
    freq = np.fft.fftshift(np.fft.fftfreq(cols))
    flt = np.abs(2 * freq)
    if name == 'ram-lak':
        ...
    elif name == 'shepp-logan':
        mid = int(cols / 2)
        flt *= np.sin(np.pi * freq) / (np.pi * freq)
        flt[mid] = 0.
    else:
        raise ValueError(f"Unknown filter name: {name}")

    if fftshift:
        flt = np.fft.ifftshift(flt)

    flt = np.tile(flt, (rows, 1))
    return flt


def apply_filter(sinogram, name):
    print(sinogram.shape[-2:])
    freq = np.fft.fft(sinogram, axis=-1)
    freq *= gen_filter(name, *sinogram.shape[-2:])
    return np.real(np.fft.ifft(freq, axis=-1))


if __name__ == "__main__":

    print(gen_filter('ram-lak', 1, 4) / 4)
    print(gen_filter('ram-lak', 1, 5) / 5)

    print(gen_filter('shepp-logan', 1, 4) / 4)
    print(gen_filter('shepp-logan', 1, 5) / 5)

    sino = np.array([
        [[1.1, 0.2, 3.5, 2.7, 1.3], [2.5, 0.1, 4.8, 5.2, 0.6]],
        [[0.4, 0.1, 2.6, 0.7, 1.5], [3.1, 0.2, 3.9, 6.7, 5.6]]
    ])

    print(apply_filter(sino, 'ram-lak'))
    print(apply_filter(sino, 'shepp-logan'))
