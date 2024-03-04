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
	    float alpha = clamp(value, 0.f, 1.f);

	    vec3 color = texture(colormap, value).xyz;

		fColor = vec4(alpha * color * (ambient + lightIntensity * diffuse), alpha);
	} else {
	    fColor = vec4(0.f);
	}
}

)glsl"