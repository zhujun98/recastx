#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/slice_mediator.hpp"

namespace tomcat::recon::test {

class SliceMediatorTest : testing::Test {

protected:

    SliceMediator mediator_;

    size_t slice_size_ = 6;

    void SetUp () {
        mediator_.resize({slice_size_, slice_size_});
    }

};

} // tomcat::recon::test