/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#include "graphics/material_manager.hpp"
#include "graphics/widgets/material_widget.hpp"

namespace recastx::gui {

MaterialManager::MaterialManager() = default;

MaterialManager &MaterialManager::instance() {
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<MaterialManager>(new MaterialManager);
    }
    return *instance_;
}

MaterialManager::~MaterialManager() = default;

template<typename T>
T* MaterialManager::createMaterial() {
    auto mat = std::make_shared<T>();
    auto id = mat->id();
    if constexpr (std::is_same_v<T, MeshMaterial>) {
        mesh_materials_[id] = mat;
        widgets_[id] = std::make_unique<MeshMaterialWidget>(mat);
    } else if constexpr (std::is_same_v<T, TransferFunc>) {
        transfer_funcs_[id] = mat;
        widgets_[id] = std::make_unique<TransferFuncWidget>(mat);
    }

    return mat.get();
}

template
MeshMaterial* MaterialManager::createMaterial<MeshMaterial>();
template
TransferFunc* MaterialManager::createMaterial<TransferFunc>();

void MaterialManager::preRender() {
    for (auto& [_, mat] : transfer_funcs_) {
        mat->updateMinMaxVals();
    }
}

} // namespace recastx::gui