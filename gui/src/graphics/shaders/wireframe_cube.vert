R"glsl(
#version 330

in vec3 vPos;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(vPos, 1.0f);
}
)glsl"