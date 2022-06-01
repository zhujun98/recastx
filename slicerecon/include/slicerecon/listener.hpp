#pragma once

#include <string>
#include <vector>
#include <variant>


namespace slicerecon {

class Reconstructor;

/// This listener construction allows the visualization server and the
/// reconstructor to talk to each other anonymously, but it is perhaps a bit
/// overengineered and unclear.
class Listener {

    friend Reconstructor;
    void register_(Reconstructor* recon);
    Reconstructor* reconstructor_ = nullptr;

public:

    virtual void notify(Reconstructor& recon) = 0;
    virtual void register_parameter(
        std::string parameter_name,
        std::variant<float, std::vector<std::string>, bool> value) = 0;

    void parameter_changed(std::string name, std::variant<float, std::string, bool> value);

};

} // slicerecon