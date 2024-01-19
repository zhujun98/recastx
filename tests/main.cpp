/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <spdlog/spdlog.h>


int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::warn);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}