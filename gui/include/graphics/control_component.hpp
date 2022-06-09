#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "object_component.hpp"

namespace gui {

class SceneObject;

class ControlComponent : public ObjectComponent {
  public:
    explicit ControlComponent(SceneObject& object);
    std::string identifier() const override { return "control"; }
    void describe() override;

    void add_float_parameter(std::string name, float value);
    void add_bool_parameter(std::string name, bool value);
    void add_enum_parameter(std::string name, std::vector<std::string> values);

  private:
    void describe_parameters_();

    // Note: scene objects are publishers, `object_->send(packet)` for parameter
    // updates
    SceneObject& object_;

    std::map<std::string, std::pair<float, std::unique_ptr<char[]>>> float_parameters_;
    std::map<std::string, bool> bool_parameters_;
    std::map<std::string, std::pair<std::vector<std::string>, std::string>> enum_parameters_;

};

} // namespace gui
