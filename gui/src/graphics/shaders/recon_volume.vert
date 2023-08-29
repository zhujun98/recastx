R"glsl(
#version 330

layout (location = 0) in vec3 vPos;

out vec3 volumeCoord;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(2.f * vPos, 1.0f);
    volumeCoord = vPos + vec3(0.5f);
}

)glsl"