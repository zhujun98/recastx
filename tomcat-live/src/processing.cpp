#include <bulk/backends/thread/thread.hpp>
#include <bulk/bulk.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <eigen3/unsupported/Eigen/FFT>

#include "processing.hpp"

void process_projection(bulk::world& world, int rows, int cols, float* data,
                        const float* dark, const float* reciproc,
                        const std::vector<float>& filter) {
    // initialize fft
    auto fft = Eigen::FFT<float>();
    auto freq_buffer = std::vector<std::complex<float>>(cols, 0);

    // divide work by rows
    int s = world.rank();
    int p = world.active_processors();
    int block_size = ((rows - 1) / p) + 1;
    int first_row = s * block_size;
    int final_row = std::min((s + 1) * block_size, rows);

    for (auto r = first_row; r < final_row; ++r) {
        int index = r * cols;
        for (auto c = 0; c < cols; ++c) {
            data[index] = (data[index] - dark[index]) * reciproc[index];
            data[index] = -std::log(data[index] <= 0.0f ? 1.0f : data[index]);
            index++;
        }

        // filter the row
        fft.fwd(freq_buffer.data(), &data[r * cols], cols);
        // ram-lak filter
        for (int i = 0; i < cols; ++i) {
           freq_buffer[i] *= filter[i];
        }
        fft.inv(&data[r * cols], freq_buffer.data(), cols);
    }

    world.barrier();
}

void process_projection_seq(int rows, int cols, float* data, const float* dark,
                            const float* reciproc, Eigen::FFT<float>& fft,
                            std::vector<std::complex<float>>& freq_buffer, const std::vector<float>& filter) {
    for (auto r = 0; r < rows; ++r) {
        int index = r * cols;
        for (auto c = 0; c < cols; ++c) {
            data[index] = (data[index] - dark[index]) * reciproc[index];
            data[index] = -std::log(data[index] <= 0.0f ? 1.0f : data[index]);
            index++;
        }

        // filter the row
        fft.fwd(freq_buffer.data(), &data[r * cols], cols);
        // ram-lak filter
        for (int i = 0; i < cols; ++i) {
               freq_buffer[i] *= filter[i];
        }
        fft.inv(&data[r * cols], freq_buffer.data(), cols);
    }
}
