R"glsl(
#version 330

in vec3 color;
out vec4 fragColor;

void main() {
    fragColor = vec4(color, 0.3f);
}
)glsl"