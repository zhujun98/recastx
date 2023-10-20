R"glsl(
#version 330

in vec2 texCoord;

out vec4 fColor;

uniform sampler1D colormap;
uniform sampler2D imageTexture;

void main() {
    float value = texture(imageTexture, texCoord).x;
    fColor = vec4(texture(colormap, value * 40).xyz, 1.0f);
}
)glsl"