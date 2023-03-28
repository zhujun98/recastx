#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "graphics/slice.hpp"

namespace recastx::gui {

Slice::Slice(int slice_id) : id_(slice_id) {}

Slice::~Slice() = default;

int Slice::id() const { return id_; }

void Slice::setData(DataType&& data, const SizeType& size) {
    data_ = std::move(data);
    size_ = size;

    updateMinMaxVal();

    texture_.setData(data_, static_cast<int>(size_[0]), static_cast<int>(size_[1]));
}

void Slice::bind() const { texture_.bind(); }
void Slice::unbind() const { texture_.unbind(); }

bool Slice::empty() const { return data_.empty(); }
bool Slice::hovered() const { return hovered_; }
bool Slice::inactive() const { return inactive_; }
bool Slice::transparent() const { return hovered_ || data_.empty(); }

void Slice::setHovered(bool state) {
    hovered_ = state;
}

void Slice::setEmpty() {
    if (!data_.empty()) {
        data_.clear();
        // I think we can leave the texture as it is.
    }
}

void Slice::setOrientation(const glm::vec3& base, const glm::vec3& x, const glm::vec3& y) {
    float orientation[16] = {x.x,  y.x,  base.x, 0.0f,  // 1
                             x.y,  y.y,  base.y, 0.0f,  // 2
                             x.z,  y.z,  base.z, 0.0f,  // 3
                             0.0f, 0.0f, 0.0f,   1.0f}; // 4

    orient_ = glm::transpose(glm::make_mat4(orientation));
}

void Slice::setOrientation(const Slice::Orient4Type& orient) {
    orient_ = orient;
}

Slice::Orient3Type Slice::orientation3() const {
    return {
        orient_[0][0], orient_[0][1], orient_[0][2],
        orient_[1][0], orient_[1][1], orient_[1][2],
        orient_[2][0], orient_[2][1], orient_[2][2]
    };
}

Slice::Orient4Type& Slice::orientation4() {
    return orient_;
}

const std::array<float, 2>& Slice::minMaxVals() const { return min_max_vals_; }

const Slice::DataType& Slice::data() const { return data_; }

void Slice::updateMinMaxVal() {
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

} // namespace recastx::gui
