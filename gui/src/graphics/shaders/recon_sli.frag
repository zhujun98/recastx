R"glsl(
#version 330

in vec2 sliceCoord;
in vec3 volumeCoord;
in vec3 fPos;

out vec4 fColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    bool isEnabled;
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2D sliceData;
uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform vec3 normal;
uniform vec3 viewPos;
uniform Material material;
uniform Light light;

uniform bool useVolumeTex;
uniform float minValue;
uniform float maxValue;

float remapMinMax(float x, float x0, float x1) {
    if (x > x1) return 1.f;
    if (x < x0) return 0.f;
    return (x - x0) / (x1 - x0);
}

void main() {
    float value;
    if (maxValue - minValue < 0.0001f) {
        value = 1.f;
    } else {
        if (useVolumeTex) {
            value = remapMinMax(texture(volumeData, volumeCoord).x, minValue, maxValue);
        } else {
            value = remapMinMax(texture(sliceData, sliceCoord).x, minValue, maxValue);
        }
    }

    vec4 color = vec4(texture(colormap, value).xyz, 1.0f);

    if (light.isEnabled) {
        vec3 lightDir = normalize(light.pos - fPos);
        vec3 viewDir = normalize(viewPos - fPos);
        vec3 halfwayDir = normalize(lightDir + viewDir);

        vec3 rgb = light.ambient * color.rgb;
        rgb += light.diffuse * color.rgb * max(dot(normal, lightDir), 0.f);
        rgb += light.specular * color.rgb * pow(max(dot(normal, halfwayDir), 0.f), 32);
        fColor = vec4(rgb, color.a);
    } else {
        fColor = color;
    }
}
)glsl"