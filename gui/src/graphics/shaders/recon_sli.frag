R"glsl(
#version 330

in vec3 fPos;
in vec2 sliceCoord;
in vec3 volumeCoord;

out vec4 fColor;

uniform vec3 sliceNormal;
uniform vec3 viewPos;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

uniform bool sampleVolumeTexture;
uniform sampler2D sliceTexture;
uniform sampler1D lutColor;
uniform sampler1D lutAlpha;
uniform sampler3D volumeTexture;

uniform float minValue;
uniform float maxValue;

uniform bool sampleVolume;

uniform bool drawFrame;
uniform vec4 frameColor;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

vec3 computeLight(Light light, vec3 color, vec3 normal, vec3 viewDir, vec3 lightDir) {
    vec3 sum = vec3(0.f, 0.f, 0.f);

    sum += light.ambient * color;
    sum += light.diffuse * color * max(dot(normal, lightDir), 0.0);
    sum += light.specular * color * pow(max(dot(normal, normalize(lightDir + viewDir)), 0.f), 16);
    return sum;
}

void main() {
    if (drawFrame) {
        fColor = frameColor;
    } else {
        float density;

        if (maxValue - minValue < 0.0001f) {
            density = 1.f;
        } else {
            if (sampleVolume) {
                density = remapMinMax(texture(volumeTexture, volumeCoord).x, minValue, maxValue);
            } else {
                density = remapMinMax(texture(sliceTexture, sliceCoord).x, minValue, maxValue);
            }
        }

        vec3 color = texture(lutColor, density).rgb;

        vec3 viewDir = normalize(viewPos - fPos);
        vec3 lightDir = normalize(light.position - fPos);

        vec3 normal = normalize(sliceNormal);
        if (dot(normal, viewDir) < 0) normal = -normal;

        fColor = vec4(computeLight(light, color, normal, viewDir, lightDir), 1.f);
    }
}
)glsl"