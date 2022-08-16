#include "graphics/volume.hpp"

namespace tomcat::gui {

Volume::Volume() : size_({8, 8, 8}) {
    setData(DataType(size_[0] * size_[1] * size_[2], 0), size_);
};

Volume::~Volume() = default;

void Volume::setData(DataType&& data, const SizeType& size) {
    data_ = data;
    size_ = size;

    updateMinMaxVal();

    texture_.setData(data_, size_[0], size_[1], size_[2]);
}

void Volume::bind() { texture_.bind(); }
void Volume::unbind() { texture_.unbind(); }

void Volume::updateMinMaxVal() {
    min_max_vals_[0] = *std::min_element(data_.begin(), data_.end());
    min_max_vals_[1] = *std::max_element(data_.begin(), data_.end());
}

const std::array<float, 2>& Volume::minMaxVals() const { return min_max_vals_; }

} // tomcat::gui