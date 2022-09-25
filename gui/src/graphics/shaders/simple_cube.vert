R"glsl(
#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 faceColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos, 1.0f);
    faceColor = aColor;
}
)glsl"