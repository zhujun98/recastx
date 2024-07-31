/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_MATERIAL_COMPONENT_H
#define GUI_MATERIAL_COMPONENT_H

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "material.hpp"

namespace recastx::gui {

class Widget;

class MaterialManager {

    std::unordered_map<MaterialID, std::shared_ptr<MeshMaterial>> mesh_materials_;
    std::unordered_map<MaterialID, std::shared_ptr<TransferFunc>> transfer_funcs_;

    std::unordered_map<MaterialID, std::unique_ptr<Widget>> widgets_;

    inline static std::unique_ptr<MaterialManager> instance_;

    MaterialManager();

  public:

    static MaterialManager& instance();

    ~MaterialManager();

    template<typename T>
    T* createMaterial();

    template<typename T>
    T* getMaterial(MaterialID id) {
        if constexpr (std::is_base_of_v<T, MeshMaterial>) return mesh_materials_.at(id).get();
        if constexpr (std::is_base_of_v<T, TransferFunc>) return transfer_funcs_.at(id).get();
        return nullptr;
    }

    Widget* getWidget(MaterialID id) {  return widgets_.at(id).get(); }

    void preRender();
};

} // namespace recastx::gui

#endif // GUI_MATERIAL_COMPONENT_H
