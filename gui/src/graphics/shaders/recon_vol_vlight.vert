R"glsl(
#version 330

layout (location = 0) in vec3 vPos;

out vec3 texCoord;

uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(2.f * vPos, 1.0f);
    texCoord = vPos + vec3(0.5f);
}

)glsl"