#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <spdlog/spdlog.h>


int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::warn);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}