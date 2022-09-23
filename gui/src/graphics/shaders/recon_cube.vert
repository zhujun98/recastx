R"glsl(
#version 330

in vec3 aPos;

out vec2 aSliceCoord;
out vec3 aVolumeCoord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 orientationMatrix;

void main() {
    aSliceCoord = vec2(aPos.xy);

    vec3 worldPosition = (orientationMatrix * vec4(aPos, 1.0f)).xyz;

    aVolumeCoord = 0.5f * (worldPosition + vec3(1.0f));

    gl_Position = projection * view * vec4(worldPosition, 1.0f);
}
)glsl"