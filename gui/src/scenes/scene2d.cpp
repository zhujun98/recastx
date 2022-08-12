#include "scenes/scene2d.hpp"
#include "scenes/scene_camera2d.hpp"
#include "shaders/shader_program.hpp"

namespace tomop::gui {

Scene2d::Scene2d() : size_{32, 32} {
    static const GLfloat square[4][2] = {{-1.0f, -1.0f},
                                         {-1.0f, 1.0f},
                                         {1.0f, 1.0f},
                                         {1.0f, -1.0f}};

    glGenVertexArrays(1, &vao_handle_);
    glBindVertexArray(vao_handle_);

    glGenBuffers(1, &vbo_handle_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), square, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    program_ = std::make_unique<ShaderProgram>("../src/shaders/simple.vert",
                                               "../src/shaders/simple.frag");
    glGenTextures(1, &texture_id_);

    camera_ = std::make_unique<SceneCamera2d>();
}

Scene2d::~Scene2d() = default;

void Scene2d::draw(glm::mat4 window_matrix) {
    program_->use();

    program_->setInt("texture_sampler", 0);
    program_->setInt("colormap_sampler", 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, camera_->colormapTextureId());

    program_->setFloat("size", 1.f / 20.f * pixel_size_);
    GLfloat aspect_ratio = (float)size_[0] / (float)size_[1];
    program_->setFloat("aspect_ratio", aspect_ratio);
    program_->setFloat("inv_aspect_ratio", 1.f / aspect_ratio);
    program_->setMat4("transform_matrix", window_matrix * camera_->matrix());

    glBindVertexArray(vao_handle_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

}  // tomop::gui
