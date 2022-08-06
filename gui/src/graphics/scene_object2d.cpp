#include "graphics/scene_camera2d.hpp"
#include "graphics/scene_object2d.hpp"
#include "graphics/shader_program.hpp"

namespace gui {

SceneObject2d::SceneObject2d() : size_{32, 32} {
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

SceneObject2d::~SceneObject2d() = default;

void SceneObject2d::draw(glm::mat4 window_matrix) {
    program_->use();

    glUniform1i(glGetUniformLocation(program_->handle(), "texture_sampler"), 0);
    glUniform1i(glGetUniformLocation(program_->handle(), "colormap_sampler"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, camera_->colormapTextureId());

    GLint loc = glGetUniformLocation(program_->handle(), "size");
    glUniform1f(loc, (1.0 / 20.0) * pixel_size_);

    auto asp = (float)size_[0] / size_[1];
    GLint loc_asp = glGetUniformLocation(program_->handle(), "aspect_ratio");
    glUniform1f(loc_asp, asp);
    GLint loc_iasp = glGetUniformLocation(program_->handle(), "inv_aspect_ratio");
    glUniform1f(loc_iasp, 1.0f / asp);

    GLint matrix_loc = glGetUniformLocation(program_->handle(), "transform_matrix");
    auto transform_matrix = window_matrix * camera_->matrix();
    glUniformMatrix4fv(matrix_loc, 1, GL_FALSE, &transform_matrix[0][0]);

    glBindVertexArray(vao_handle_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

}  // namespace gui
