R"glsl(
#version 330

in vec3 position;

out vec2 sliceCoord;
out vec3 volumeCoord;

uniform mat4 worldToScreenMatrix;
uniform mat4 orientationMatrix;

void main() {
    sliceCoord = vec2(position.x, position.y);

    vec3 worldPosition = (orientationMatrix * vec4(position, 1.0f)).xyz;

    volumeCoord = 0.5f * (worldPosition + vec3(1.0f));

    gl_Position = worldToScreenMatrix * vec4(worldPosition, 1.0f);
}
)glsl"