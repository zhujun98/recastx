#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/application.hpp"

namespace tomcat::recon::test {

class CliceBufferBridgeTest : testing::Test {

protected:

    SliceBufferBridge buffer_;

    size_t slice_size_ = 6;

    void SetUp () {
        buffer_.resize({slice_size_, slice_size_});
    }

};

} // tomcat::recon::test