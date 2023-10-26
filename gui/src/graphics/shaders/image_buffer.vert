R"glsl(
#version 330

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 vTexCoord;

out vec2 texCoord;

void main() {
    gl_Position = vec4(vPos, 1.0f);
    texCoord = vTexCoord;
}
)glsl"