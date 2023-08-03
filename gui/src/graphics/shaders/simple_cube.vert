R"glsl(
#version 330

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;

out vec3 faceColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(vPos, 1.0f);
    faceColor = vColor;
}
)glsl"