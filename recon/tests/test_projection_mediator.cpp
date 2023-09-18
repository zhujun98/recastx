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
    auto& projections = m.projections();

    m.emplace(Projection(ProjectionType::PROJECTION, 1, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.emplace(Projection(ProjectionType::PROJECTION, 2, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    m.emplace(Projection(ProjectionType::PROJECTION, 3, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    ASSERT_TRUE(projections.fetch(-1));
    ASSERT_THAT(projections.front(), ElementsAre(4, 3, 2, 1));
    ASSERT_TRUE(projections.fetch(-1));
    ASSERT_THAT(projections.front(), ElementsAre(1, 2, 3, 4));
    ASSERT_FALSE(projections.fetch(0));
    
    m.emplace(Projection(ProjectionType::PROJECTION, 3, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.reset();
    ASSERT_FALSE(projections.fetch(0));
}

TEST(ProjectionMediatorTest, TestSetFilter1) {

    ProjectionMediator m;
    m.setFilter(10, 2);
    auto& projections = m.projections();

    m.emplace(Projection(ProjectionType::PROJECTION, 1, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    ASSERT_FALSE(projections.fetch(0));

    m.emplace(Projection(ProjectionType::PROJECTION, 12, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    ASSERT_TRUE(projections.fetch(-1));
    ASSERT_THAT(projections.front(), ElementsAre(1, 2, 3, 4));
}

TEST(ProjectionMediatorTest, TestSetFilter2) {

    ProjectionMediator m;
    m.setFilter(-5, 2);
    auto& projections = m.projections();

    m.emplace(Projection(ProjectionType::PROJECTION, 1, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.emplace(Projection(ProjectionType::PROJECTION, 2, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    ASSERT_TRUE(projections.fetch(0));
    ASSERT_TRUE(projections.fetch(0));
    ASSERT_FALSE(projections.fetch(0));
}

TEST(ProjectionMediatorTest, TestSetFilter3) {

    ProjectionMediator m;
    m.setFilter(5, -6);
    auto& projections = m.projections();

    m.emplace(Projection(ProjectionType::PROJECTION, 0, 2, 2, std::vector<RawDtype>{1, 2, 3, 4}));
    m.emplace(Projection(ProjectionType::PROJECTION, 5, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    m.emplace(Projection(ProjectionType::PROJECTION, 6, 2, 2, std::vector<RawDtype>{4, 3, 2, 1}));
    ASSERT_TRUE(projections.fetch(0));
    ASSERT_TRUE(projections.fetch(0));
    ASSERT_FALSE(projections.fetch(0));
}

}