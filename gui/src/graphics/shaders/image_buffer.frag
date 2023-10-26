R"glsl(
#version 330

in vec2 texCoord;

out vec4 fColor;

uniform sampler1D colormap;
uniform sampler2D imageTexture;
uniform float minValue;
uniform float maxValue;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

void main() {
    float value;
    if (minValue == maxValue) {
        value = 1.f;
    } else {
        value = remapMinMax(texture(imageTexture, texCoord).x, minValue, maxValue);
    }

    fColor = vec4(texture(colormap, value).xyz, 1.0f);
}
)glsl"