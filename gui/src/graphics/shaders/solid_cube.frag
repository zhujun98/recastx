R"glsl(
#version 330

in vec2 aSliceCoord;
in vec3 aVolumeCoord;

out vec4 fragColor;

uniform sampler2D sliceData;
uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform int hovered;
uniform int empty;
uniform float minValue;
uniform float maxValue;

float remapMinMax(float x, float x0, float x1) {
    return (x - x0) / (x1 - x0);
}

void main() {
    float value;
    if (minValue == maxValue) {
        value = 1.f;
    } else {
        if (empty == 1) {
            value = remapMinMax(texture(volumeData, aVolumeCoord).x, minValue, maxValue);
        } else {
            value = remapMinMax(texture(sliceData, aSliceCoord).x, minValue, maxValue);
        }
    }

    fragColor = vec4(texture(colormap, value).xyz, 1.0f);

    if (empty == 1) {
        fragColor.a = 0.75f;
    }

    if (hovered == 1) {
        fragColor += vec4(0.3f);
    }
}
)glsl"