R"glsl(
#version 330 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;

out vec3 fPos;
out vec2 fTexCoord;
out vec3 fNormal;

uniform mat4 model;
uniform mat4 mvp;

void main() {
    fPos = vec3(model * vec4(vPos, 1.0)); // to world space
    fNormal = mat3(transpose(inverse(model))) * vNormal;
    fTexCoord = vTexCoord;

    gl_Position = mvp * vec4(vPos, 1.0);
}
)glsl"