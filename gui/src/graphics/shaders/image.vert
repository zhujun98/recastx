R"glsl(
#version 330

layout (location = 0) in vec3 vPos;

out vec2 texCoord;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(vPos, 1.0f);
    texCoord = 0.5f * vPos.xy + 0.5f;
}
)glsl"