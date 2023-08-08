R"glsl(
#version 330

in vec3 volumeCoord;

out vec4 fColor;

uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform float alpha;
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
        value = remapMinMax(texture(volumeData, volumeCoord).x, minValue, maxValue);
    }

    fColor = vec4(texture(colormap, value).xyz, alpha);
}
)glsl"