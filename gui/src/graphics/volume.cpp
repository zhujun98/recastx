#include "graphics/volume.hpp"

namespace tomcat::gui {

Volume::Volume() : size_({128, 128, 128}) {
    DataType data(size_[0] * size_[1] * size_[2], 0);
    setData(std::move(data), size_);
};

Volume::~Volume() = default;

void Volume::setData(DataType&& data, const SizeType& size) {
    data_ = std::move(data);
    size_ = size;

    updateMinMaxVal();

    texture_.setData(data_,
                     static_cast<int>(size_[0]),
                     static_cast<int>(size_[1]),
                     static_cast<int>(size_[2]));
}

void Volume::bind() const { texture_.bind(); }
void Volume::unbind() const { texture_.unbind(); }

void Volume::updateMinMaxVal() {
    auto [vmin, vmax] = std::minmax_element(data_.begin(), data_.end());
    min_max_vals_ = {*vmin, *vmax};
}

const std::array<float, 2>& Volume::minMaxVals() const { return min_max_vals_; }

} // namespace tomcat::gui