/**
 * Copyright (c) Paul Scherrer Institut
 * Author: Jun Zhu
 *
 * Distributed under the terms of the BSD 3-Clause License.
 *
 * The full license is in the file LICENSE, distributed with this software.
*/
#ifndef GUI_VOXEL_OBJECT_H
#define GUI_VOXEL_OBJECT_H

#include "renderable.hpp"
#include "textures.hpp"
#include "volume_slicer.hpp"

namespace recastx::gui {

class VoxelObject : public Renderable {

    class Model;

    class Framebuffer {

        GLuint VAO_;
        GLuint VBO_;

        int width_;
        int height_;
        GLuint FBO_;

        GLuint light_texture_;
        GLuint eye_texture_;

        void createTexture(GLuint& texture_id);

      public:

        Framebuffer(int width, int height);

        ~Framebuffer();

        void bind() const;

        friend class Model;
    };

    class Model : public VolumeSlicer {

        GLuint VAO_;
        GLuint VBO_;

        void init();

      public:

        explicit Model(size_t num_slices);

        ~Model() override;

        void renderOnFramebuffer(Framebuffer* fb,
                                 ShaderProgram* vslice_shader,
                                 ShaderProgram* vlight_shader,
                                 bool is_view_inverted);

        void renderFramebufferOnScreen(Framebuffer* fb);

        void render();

        void setNumSlices(size_t num_slices);
    };

    bool volume_shadow_;
    int num_slices_;
    const int NUM_SLICES0;
    std::unique_ptr<Model> model_;
    std::unique_ptr<Framebuffer> fb_;

    Texture3D intensity_;

    float threshold_;

    std::unique_ptr<ShaderProgram> vslice_shader_;
    std::unique_ptr<ShaderProgram> vlight_shader_;
    std::unique_ptr<ShaderProgram> vscreen_shader_;

    void renderVolumeShadow(Renderer* renderer);

    void renderSimple(Renderer* renderer);

    static int getNumSlices(RenderQuality value);

    void setNumSlices(int num_slices) {
        num_slices_ = num_slices;
        model_->setNumSlices(num_slices);
    }

  public:

    VoxelObject();

    void render(Renderer* renderer) override;

    void renderGUI() override;

    template<typename T>
    void setIntensity(const T* data, uint32_t x, uint32_t y, uint32_t z) {
        intensity_.setData(data, x, y, z);
    }

    void resetIntensity() {
        intensity_.invalidate();
    }

    Texture3D* intensity() { return &intensity_; }

    void setRenderQuality(RenderQuality value);

    void setThreshold(float threshold) { threshold_ = threshold; }
};

} // recastx::gui

#endif // GUI_VOXEL_OBJECT_H