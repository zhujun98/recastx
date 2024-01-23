R"glsl(
#version 330

layout(location = 0) out vec4 fColor;

smooth in vec3 texCoords;
smooth in vec4 shadowCoords;

uniform sampler3D volumeData;
uniform sampler2D shadowTexture;
uniform vec3 lightColor;
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

	float density = remapMinMax(texture(volumeData, texCoords).r, minValue, maxValue);

	if(density > threshold) {
		fColor = vec4(lightColor * lightIntensity * density, density);
	}
}

)glsl"