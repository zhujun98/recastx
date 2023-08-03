R"glsl(
#version 330

in vec3 lineColor;

out vec4 fColor;

void main() {
    fColor = vec4(lineColor, 1.0f);
}
)glsl"