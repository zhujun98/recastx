R"glsl(
#version 330

in vec2 glyphCoords;

out vec4 color;

uniform sampler2D glyphTexture;
uniform vec3 glyphColor;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(glyphTexture, glyphCoords).r);
    color = vec4(glyphColor, 1.0) * sampled;
}
)glsl"