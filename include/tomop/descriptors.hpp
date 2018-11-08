#pragma once

namespace tomop {

enum class packet_desc : int {
    // SCENE MANAGEMENT
    make_scene,
    kill_scene,

    // RECONSTRUCTION
    slice_data,
    partial_slice_data,
    volume_data,
    partial_volume_data,
    set_slice,
    remove_slice,
    group_request_slices,

    // GEOMETRY
    geometry_specification,
    scan_settings,
    parallel_beam_geometry,
    parallel_vec_geometry,
    cone_beam_geometry,
    cone_vec_geometry,
    projection_data,
    partial_projection_data,
    projection,

    // PARTITIONING
    set_part,
};

}  // namespace tomop
