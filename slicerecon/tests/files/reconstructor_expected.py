import numpy as np
np.set_printoptions(precision=8)

shape = (3, 4, 3)

darks = np.array([
    4, 1, 1, 2, 0, 9, 7, 4, 3, 8, 6, 8, 
    1, 7, 3, 0, 6, 6, 0, 8, 1, 8, 4, 2, 
    2, 4, 6, 0, 9, 5, 8, 3, 4, 2, 2, 0
], dtype=np.uint16)

flats = np.array([
    1, 9, 5, 1, 7, 9, 0, 6, 7, 1, 5, 6, 
    2, 4, 8, 1, 3, 9, 5, 6, 1, 1, 1, 7, 
    9, 9, 4, 1, 6, 8, 6, 9, 2, 4, 9, 4
], dtype=np.uint16)

dark_avg = np.mean(darks.reshape(shape), axis=0, dtype=np.float32)
flat_avg = np.mean(flats.reshape(shape), axis=0, dtype=np.float32)

print(dark_avg)
print(1. / (flat_avg - dark_avg))