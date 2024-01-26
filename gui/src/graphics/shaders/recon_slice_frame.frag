R"glsl(
#version 330

out vec4 fColor;

uniform vec4 frameColor;

void main() {
    fColor = frameColor;
}
)glsl"