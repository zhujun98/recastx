#pragma once


namespace gui {

template <typename T>
struct parameter {
    std::string name;
    T min_value;
    T max_value;
    T* value;
};

} // namespace gui
