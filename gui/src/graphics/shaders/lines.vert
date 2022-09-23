R"glsl(
#version 330

layout (location = 0) in vec3 aPos;

out vec3 aColor;

uniform float scale;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(aPos.x * scale,
                                           aPos.y * scale,
                                           aPos.z * scale,
                                           1.0f);
    aColor = aPos;
}
)glsl"