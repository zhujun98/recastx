R"glsl(

#version 330

layout(location = 0) out vec4 fColor;

in vec3 texCoords;

uniform sampler3D volumeTexture;
uniform sampler1D lutColor;
uniform sampler1D lutAlpha;

uniform vec3 ambient;
uniform vec3 diffuse;
uniform float threshold;
uniform float samplingRate;

uniform float minValue;
uniform float maxValue;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

void main() {
    float density;
    if (maxValue - minValue < 0.0001f) {
        density = 1.f;
    } else {
        density = remapMinMax(texture(volumeTexture, texCoords).r, minValue, maxValue);
    }

	if (density > threshold) {
        vec3 color = texture(lutColor, density).rgb;
        float alpha = texture(lutAlpha, density).r;
        alpha = 1 - pow(1 - alpha, samplingRate);
        fColor = vec4(alpha * color * (ambient + diffuse), alpha);
    } else {
        fColor = vec4(0.f);
    }
}

)glsl"