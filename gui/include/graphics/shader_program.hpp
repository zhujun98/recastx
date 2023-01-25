#ifndef GUI_SHADERPROGRAM_H
#define GUI_SHADERPROGRAM_H

#include <string>

#include "GL/gl3w.h"
#include "glm/glm.hpp"

namespace tomcat::gui {

class ShaderProgram {

    GLuint program_ = 0;

    void checkCompileErrors(GLuint shader, const std::string& type);

    void compileShaderProgram(const char* vcode, const char* fcode);

  public:

    ShaderProgram(const char* vcode, const char* fcode);

    ShaderProgram(const std::string& vfile_path, const std::string& ffile_path);

    ~ShaderProgram();

    void use() const;

    void setMat4(const std::string& name, glm::mat4 value);
    void setVec3(const std::string& name, glm::vec3 value);
    void setVec4(const std::string& name, glm::vec4 value);
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    void setBool(const std::string& name, bool value);

};

} // namespace tomcat::gui

#endif // GUI_SHADERPROGRAM_H