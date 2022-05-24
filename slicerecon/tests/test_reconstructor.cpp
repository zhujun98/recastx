#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include "slicerecon/slicerecon.hpp"

namespace slicerecon::test {

TEST(TestReconstructor, general) {
    auto paganin = slicerecon::PaganinSettings {};
    auto params = slicerecon::Settings {
        128, 128, 128, 8, 
        10, 10, 128, 
        slicerecon::ReconstructMode::alternating,
        false, false, false, paganin, "shepp-logan"
    };
    auto geom = slicerecon::Geometry {
        300, 200, 128,
        {}, true, false,
        {0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        0.0f,
        0.0f };
    slicerecon::Reconstructor recon(params, geom);
}

} // slicerecon::test