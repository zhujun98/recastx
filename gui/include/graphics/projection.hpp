/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_PROJECTION_H
#define GUI_PROJECTION_H

#include <array>
#include <vector>

#include "GL/gl3w.h"

#include "common/config.hpp"
#include "graphics/textures.hpp"

namespace recastx::gui {

class Colormap;
class ImageBuffer;

class Projection {

  public:

    using DataType = std::vector<RawDtype>;

  private:

    uint32_t id_;

    std::unique_ptr<ImageBuffer> buffer_;

    std::unique_ptr<Colormap> cm_;

    ImageTexture<RawDtype> texture_;

  public:

    explicit Projection(uint32_t id);
    ~Projection();

    void setData(const DataType& data, const std::array<size_t, 2>& size);

    void resize(int width, int height);

    [[nodiscard]] uint32_t id() const { return id_; }

    [[nodiscard]] GLuint texture() const;
};

} // namespace recastx::gui

#endif // GUI_PROJECTION_H
