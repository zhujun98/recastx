# -----------------------------------------------------------------------------
# Copyright (c) Paul Scherrer Institut
# Author: Jun Zhu
#
# Distributed under the terms of the BSD 3-Clause License.
#
# The full license is in the file LICENSE, distributed with this software.
# -----------------------------------------------------------------------------
add_library(recastx_models cube_model.cpp)

target_include_directories(recastx_models PRIVATE ./include)
