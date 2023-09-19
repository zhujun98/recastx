R"glsl(
#version 330

in vec2 sliceCoord;
in vec3 volumeCoord;

out vec4 fColor;

uniform sampler2D sliceData;
uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform int hovered;
uniform int empty;
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
        if (empty == 1) {
            value = remapMinMax(texture(volumeData, volumeCoord).x, minValue, maxValue);
        } else {
            value = remapMinMax(texture(sliceData, sliceCoord).x, minValue, maxValue);
        }
    }

    fColor = vec4(texture(colormap, value).xyz, 1.0f);

    if (empty == 1) {
        fColor.a = 0.75f;
    }

    if (hovered == 1) {
        fColor += vec4(0.3f);
    }
}
)glsl"