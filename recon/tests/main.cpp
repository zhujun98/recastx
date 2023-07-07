/**
 * Copyright (c) Paul Scherrer Institut PSI
 * Author: Jun Zhu
 *
 * Distributed under the terms of the GPLv3 License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <spdlog/spdlog.h>


int main(int argc, char **argv) {
  //spdlog::set_level(spdlog::level::warn);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}