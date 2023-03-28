#include <fstream>
#include <iostream>
#include <sstream>

#include "graphics/shader_program.hpp"

namespace recastx::gui {

ShaderProgram::ShaderProgram(const char* vcode, const char* fcode) {
    compileShaderProgram(vcode, fcode);
}

ShaderProgram::ShaderProgram(const std::string& vfile_path, const std::string& ffile_path) {

    std::string vcode_str;
    std::string fcode_str;
    std::ifstream vfile;
    std::ifstream ffile;

    vfile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    ffile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        vfile.open(vfile_path);
        ffile.open(ffile_path);
        std::stringstream vcode_stream, fcode_stream;

        vcode_stream << vfile.rdbuf();
        fcode_stream << ffile.rdbuf();

        vfile.close();
        ffile.close();

        vcode_str = vcode_stream.str();
        fcode_str = fcode_stream.str();
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }

    const char* vcode = vcode_str.c_str();
    const char * fcode = fcode_str.c_str();
    compileShaderProgram(vcode, fcode);
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(program_);
}

void ShaderProgram::use() const { glUseProgram(program_); }

void ShaderProgram::setMat4(const std::string& name, glm::mat4 value){
    glUniformMatrix4fv(glGetUniformLocation(program_, name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void ShaderProgram::setVec3(const std::string& name, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(program_, name.c_str()), 1, &value[0]);
}

void ShaderProgram::setVec4(const std::string& name, glm::vec4 value) {
    glUniform4fv(glGetUniformLocation(program_, name.c_str()), 1, &value[0]);
}

void ShaderProgram::setFloat(const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(program_, name.c_str()), value);
}

void ShaderProgram::setInt(const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(program_, name.c_str()), value);
}

void ShaderProgram::setBool(const std::string& name, bool value) {
    glUniform1i(glGetUniformLocation(program_, name.c_str()), static_cast<int>(value));
}

void ShaderProgram::checkCompileErrors(GLuint shader, const std::string& type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog
                      << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog
                      << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

void ShaderProgram::compileShaderProgram(const char* vcode, const char* fcode) {
    GLuint vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vcode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fcode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    program_ = glCreateProgram();
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    checkCompileErrors(program_, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

} // namespace recastx::gui
