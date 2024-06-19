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

    void genCylinder(uint32_t xc, uint32_t yc, uint32_t z0, uint32_t z1, uint32_t r, DataType::value_type v);

    void genDonut(uint32_t xc, uint32_t yc, uint32_t zc, uint32_t r1, uint32_t r2, DataType::value_type v);

    void genSphere(uint32_t xc, uint32_t yc, uint32_t zc, uint32_t r, DataType::value_type  v);

  public:

    explicit CubeModel(uint32_t x);

    CubeModel(uint32_t x, uint32_t y, uint32_t z);

    void genSpheres(size_t n);

    [[nodiscard]] std::string data() const;

    [[nodiscard]] uint32_t x() const { return x_; }
};

#endif //MODEL_CUBEMODEL_H