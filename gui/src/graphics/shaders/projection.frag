R"glsl(
#version 330

in vec2 texCoord;

out vec4 fColor;

uniform sampler1D colormap;
uniform sampler2D projectionTexture;

void main() {
    float value = texture(projectionTexture, texCoord).x;
    fColor = vec4(texture(colormap, value).xyz, 1.0f);
}
)glsl"