#include "graphics/components/scene_component.hpp"

namespace tomcat::gui {

// class SceneComponent

SceneComponent::SceneComponent(SceneComponent::ComponentType type, Scene& scene)
    : type_(type), scene_(scene) {}

SceneComponent::~SceneComponent() = default;

void SceneComponent::init() {}

void SceneComponent::onWindowSizeChanged(int width, int height) {}

// class StaticSceneComponent

StaticSceneComponent::StaticSceneComponent(Scene &scene)
        : SceneComponent(SceneComponent::ComponentType::STATIC, scene) {}

StaticSceneComponent::~StaticSceneComponent() = default;

// class DynamicSceneComponent

DynamicSceneComponent::DynamicSceneComponent(Scene &scene)
    : SceneComponent(SceneComponent::ComponentType::DYNAMIC, scene) {}

DynamicSceneComponent::~DynamicSceneComponent() = default;

} // namespace tomcat::gui