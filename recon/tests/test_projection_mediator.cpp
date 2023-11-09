/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "recon/projection_mediator.hpp"

namespace recastx::recon::test {

using ::testing::ElementsAre;

TEST(ProjectionMediatorTest, TestDefault) {

    ProjectionMediator m(2);

    m.push(Projection(ProjectionType::PROJECTION, 1, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.push(Projection(ProjectionType::PROJECTION, 2, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    m.push(Projection(ProjectionType::PROJECTION, 3, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));

    Projection proj;
    ASSERT_TRUE(m.waitAndPop(proj));
    ASSERT_THAT(proj.data, ElementsAre(4, 3, 2, 1));
    ASSERT_TRUE(m.waitAndPop(proj));
    ASSERT_THAT(proj.data, ElementsAre(1, 2, 3, 4));
    ASSERT_FALSE(m.waitAndPop(proj, 0));
    
    m.push(Projection(ProjectionType::PROJECTION, 3, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.reset();
    ASSERT_FALSE(m.waitAndPop(proj, 0));
}

TEST(ProjectionMediatorTest, TestSetFilter1) {

    ProjectionMediator m;
    m.setFilter(10);
    m.setId(2);

    Projection proj;
    m.push(Projection(ProjectionType::PROJECTION, 1, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    ASSERT_FALSE(m.waitAndPop(proj, 0));

    m.push(Projection(ProjectionType::PROJECTION, 12, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    ASSERT_TRUE(m.waitAndPop(proj));
    ASSERT_THAT(proj.data, ElementsAre(1, 2, 3, 4));
}

TEST(ProjectionMediatorTest, TestSetFilter3) {

    ProjectionMediator m;
    m.setFilter(5);
    m.setId(5);

    Projection proj;
    m.push(Projection(ProjectionType::PROJECTION, 0, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.push(Projection(ProjectionType::PROJECTION, 4, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    m.push(Projection(ProjectionType::PROJECTION, 5, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    ASSERT_TRUE(m.waitAndPop(proj, 0));
    ASSERT_FALSE(m.waitAndPop(proj, 0));
}

}