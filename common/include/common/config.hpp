#ifndef TOMCAT_H
#define TOMCAT_H

namespace tomcat {

    constexpr size_t MAX_NUM_SLICES = 3;

    using Orientation = std::array<float, 9>;

    enum class ProjectionType : int32_t { dark = 0, flat = 1, projection = 2, unknown = 99 };

    using RawDtype = uint16_t;
    using ProDtype = float;

    struct DaqClientConfig {
        std::string hostname;
        int port;
        std::string socket_type;
    };

    struct ZmqServerConfig {
        int data_port;
        int message_port;
    };

    struct FilterConfig {
        std::string name;
        bool gaussian_lowpass_filter;
    };

    struct PaganinConfig {
        float pixel_size;
        float lambda;
        float delta;
        float beta;
        float distance;
    };

    struct ProjectionGeometry {
        size_t col_count; // number of detector columns
        size_t row_count; // number of detector rows
        float pixel_width; // width of each detector
        float pixel_height; // height of each detector
        std::vector<float> angles; // array of projection angles 
        float source2origin = 0.f;
        float origin2detector = 0.f;
    };

    struct VolumeGeometry {
        size_t col_count; // number of columns
        size_t row_count; // number of rows
        size_t slice_count; // number of slices
        float min_x; // minimum x coordinates
        float max_x; // maximum x coordinates
        float min_y; // minimum y coordinates
        float max_y; // maximum y coordinates
        float min_z; // minimum z coordinates
        float max_z; // maximum z coordinates
    };

} // tomcat

#endif // TOMCAT_H