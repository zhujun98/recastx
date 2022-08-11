#include <glm/gtc/type_ptr.hpp>

#include "graphics/slice.hpp"
#include "math_common.hpp"
#include "util.hpp"

namespace gui {

Slice::Slice(int slice_id) : id_(slice_id), size_{32, 32}, texture_(32, 32) {
    updateTexture();
}

int Slice::id() const { return id_; }

void Slice::setData(const DataType& data, const SizeType& size) {
    data_ = data;
    size_ = size;

    updateMinMaxVal();
}

void Slice::updateTexture() {
    if (data_.empty()) return;

    texture_.setData(data_, size_[0], size_[1]);
}

bool Slice::empty() const { return data_.empty(); }
bool Slice::hovered() const { return hovered_; }
bool Slice::inactive() const { return inactive_; }
bool Slice::transparent() const { return hovered_ || data_.empty(); }

void Slice::setHovered(bool state) {
    hovered_ = state;
}

void Slice::setOrientation(glm::vec3 base, glm::vec3 x, glm::vec3 y) {
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

const std::array<float, 2>& Slice::minMaxVals() const { return minMaxVals_; }

void Slice::updateMinMaxVal() {
    minMaxVals_[0] = *std::min_element(data_.begin(), data_.end());
    minMaxVals_[1] = *std::max_element(data_.begin(), data_.end());
}

} // namespace gui
