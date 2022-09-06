#include <xtensor/xadapt.hpp>
#include <xtensor/xmath.hpp>

#include "graphics/volume.hpp"

namespace tomcat::gui {

// TODO: Use the default preview size as the edge length.
Volume::Volume() : size_({128, 128, 128}) {
    DataType data(size_[0] * size_[1] * size_[2], -1);
    // Non-uniform initialization is good for colormap display test.
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<float>(i % 128);
    setData(std::move(data), size_);
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
    min_max_vals_ = xt::minmax(xt::adapt(data_, {data_.size()}))();
}

const std::array<float, 2>& Volume::minMaxVals() const { return min_max_vals_; }

} // namespace tomcat::gui