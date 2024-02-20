R"glsl(
#version 330

in vec3 vPos;

out vec3 fPos;
out vec2 sliceCoord;
out vec3 volumeCoord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 orientationMatrix;

void main() {
    sliceCoord = vec2(vPos.xy);

    fPos = (orientationMatrix * vec4(vPos, 1.0f)).xyz;

    volumeCoord = 0.5f * (fPos + vec3(1.0f));

    gl_Position = projection * view * vec4(fPos, 1.0f);
}
)glsl"