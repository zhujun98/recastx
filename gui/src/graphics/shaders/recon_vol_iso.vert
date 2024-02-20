R"glsl(
#version 330

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNorm;

out vec3 fPos;
out vec3 texCoord;
out vec3 outNorm;

uniform mat4 mvp;

void main() {
    fPos = 2.f * vPos - 1.f;
    gl_Position = mvp * vec4(fPos, 1.0f);
    outNorm = vNorm;
    texCoord = vPos;
}

)glsl"