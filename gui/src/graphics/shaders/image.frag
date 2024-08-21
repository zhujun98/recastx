R"glsl(
#version 330

in vec2 texCoord;

out vec4 fColor;

uniform usampler2D imageTexture;
uniform sampler1D lutColor;
uniform float minValue;
uniform float maxValue;

uniform vec4 frameColor;
uniform bool drawFrame;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

void main() {
    if (drawFrame) {
        fColor = frameColor;
    } else {
        float value;
        if (maxValue - minValue < 0.0001f) {
            value = 1.f;
        } else {
            value = remapMinMax(float(texture(imageTexture, texCoord).x), minValue, maxValue);
        }
        fColor = vec4(texture(lutColor, value).xyz, 1.0f);
    }
}
)glsl"