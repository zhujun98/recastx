R"glsl(
#version 330

in vec3 vPos;

out vec3 fPos;
out vec2 sliceCoord;
out vec3 volumeCoord;

uniform mat4 mvp;
uniform mat4 model;

void main() {
    sliceCoord = vPos.xy + vec2(0.5f);

    fPos = (model * vec4(vPos, 1.0f)).xyz;

    volumeCoord = fPos + vec3(0.5f);

    gl_Position = mvp * vec4(vPos, 1.0f);
}
)glsl"