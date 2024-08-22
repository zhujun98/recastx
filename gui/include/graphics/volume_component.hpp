/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOLUME_H
#define GUI_VOLUME_H

#include <array>
#include <memory>
#include <optional>

#include "style.hpp"
#include "textures.hpp"
#include "fps_counter.hpp"

#include "component.hpp"
#include "mesh_object.hpp"
#include "data.hpp"

namespace recastx::gui {

class ShaderProgram;
class Marcher;
class VoxelObject;
class MeshObject;
class SliceComponent;

class VolumeComponent : public Component {

  public:

    enum RenderPolicy { VOXEL = 0, SURFACE = 1 };
    enum DisplayPolicy { PREVIEW = 0, SHOW = 1, DISABLE = 2};

  private:

    using DataType = Data3D<ProDtype>;

    DataType data_;
    DataType buffer_;

    mutable std::mutex mtx_;

    VoxelObject* voxel_object_ = nullptr;
    bool update_texture_ = true;

    MeshObject* mesh_object_ = nullptr;
    bool update_mesh_ = true;

    std::unique_ptr<Marcher> marcher_;

    int render_quality_;
    int render_policy_ = VOXEL;
    int display_policy_ = PREVIEW;
    float iso_value_;

    SliceComponent* slice_comp_;

    void setData(const std::string& data, uint32_t x, uint32_t y, uint32_t z);

  public:

    explicit VolumeComponent(RpcClient* client);

    ~VolumeComponent() override;

    void setMeshObject(MeshObject* obj);

    void setVoxelObject(VoxelObject* obj);

    void draw(rpc::ServerState_State state) override;

    bool drawStatistics(rpc::ServerState_State state) const;

    void preRender() override;

    RpcClient::State updateServerParams() const override;

    bool setShard(uint32_t pos, const std::string& data, uint32_t x, uint32_t y, uint32_t z);

    void setRenderQuality(RenderQuality quality);

    void setSliceComponent(SliceComponent* ptr) { slice_comp_ = ptr; }

    void onShowSlice();
};

} // namespace recastx::gui

#endif // GUI_VOLUME_H
