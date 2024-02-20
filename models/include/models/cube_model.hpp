/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef MODEL_CUBEMODEL_H
#define MODEL_CUBEMODEL_H

#include <string>
#include <vector>

class CubeModel {

  public:

    using DataType = std::vector<float>;

  private:

    uint32_t x_;
    uint32_t y_;
    uint32_t z_;
    DataType data_;

    void genSubData(uint32_t x0, uint32_t y0, uint32_t z0, size_t step_size);

    void genData();

  public:

    CubeModel();

    [[nodiscard]] std::string data() const;

    [[nodiscard]] uint32_t x() const { return x_; }

    [[nodiscard]] uint32_t y() const { return y_; }

    [[nodiscard]] uint32_t z() const { return z_; }
};

#endif //MODEL_CUBEMODEL_H