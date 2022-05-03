#include <slicerecon/reconstruction/listener.hpp>
#include <slicerecon/reconstruction/reconstructor.hpp>


namespace slicerecon {

void Listener::parameter_changed(std::string name, std::variant<float, std::string, bool> value) {
    reconstructor_->parameterChanged(name, value);
}

void Listener::register_(Reconstructor* recon) {
    reconstructor_ = recon;
}

}