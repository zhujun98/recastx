R"glsl(
#version 330

layout (location = 0) in vec2 vPos;
layout (location = 1) in vec2 vTexCoords;

out vec2 texCoords;

void main() {
    texCoords = vTexCoords;
    gl_Position = vec4(vPos.xy, 0.f, 1.f);
}

)glsl"