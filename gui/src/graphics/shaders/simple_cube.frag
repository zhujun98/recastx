R"glsl(
#version 330

in vec3 faceColor;

out vec4 fragColor;

void main() {
    fragColor = vec4(faceColor, 1.0f);
}
)glsl"