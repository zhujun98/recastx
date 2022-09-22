R"glsl(
#version 330

in vec3 aColor;

out vec4 fragColor;

void main() {
    fragColor = vec4(aColor, 0.3f);
}
)glsl"