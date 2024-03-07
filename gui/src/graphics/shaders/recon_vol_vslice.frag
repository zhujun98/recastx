R"glsl(
#version 330

layout(location = 0) out vec4 fColor;

smooth in vec3 texCoords;
smooth in vec4 shadowCoords;

uniform sampler1D colormap;
uniform sampler3D volumeData;
uniform sampler2D shadowTexture;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform float threshold;

uniform float minValue;
uniform float maxValue;
uniform float alphaScale;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

void main() {
    vec3 lightIntensity =  textureProj(shadowTexture, shadowCoords.xyw).xyz;

    float value;
    if (maxValue - minValue < 0.0001f) {
        value = 1.f;
    } else {
        value = remapMinMax(texture(volumeData, texCoords).r, minValue, maxValue);
    }

	if (value > threshold) {
	    vec3 color = texture(colormap, value).rgb;

		fColor = vec4(value * color * (ambient + lightIntensity * diffuse), value * alphaScale);
	} else {
	    fColor = vec4(0.f);
	}
}

)glsl"