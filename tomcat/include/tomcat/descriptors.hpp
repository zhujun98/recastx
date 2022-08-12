#pragma once

namespace tomcat {

enum class PacketDesc : int {
    // SCENE MANAGEMENT
    make_scene = 0x101,
    kill_scene = 0x102,

    // RECONSTRUCTION
    slice_data = 0x201,
    volume_data = 0x203,
    set_slice = 0x205,
    remove_slice = 0x206,

    // GEOMETRY
    geometry_specification = 0x301,
    scan_settings = 0x302,
    parallel_beam_geometry = 0x303,
    parallel_vec_geometry = 0x304,
    cone_beam_geometry = 0x305,
    cone_vec_geometry = 0x306,
    projection_data = 0x307,
    partial_projection_data = 0x308,
    projection = 0x309,

    // PARTITIONING
    set_part = 0x401,

    // CONTROL
    parameter_bool = 0x501,
    parameter_float = 0x502,
    parameter_enum = 0x503,
};

} // namespace tomcat
