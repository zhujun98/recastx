R"glsl(
#version 330

in vec3 fPos;
in vec3 texCoord;
in vec3 outNorm;

out vec4 fColor;

struct Light {
    bool isEnabled;
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform vec3 viewPos;
uniform Light light;

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
        density = remapMinMax(texture(volumeData, texCoord).x, minValue, maxValue);
    }
    vec3 value = texture(colormap, density).xyz;

    vec3 lightDir = normalize(light.pos - fPos);
    float diff = max(dot(lightDir, outNorm), 0.0);

    vec3 viewDir = normalize(viewPos - fPos);
    vec3 reflectDir = reflect(-lightDir, outNorm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(outNorm, halfwayDir), 0.0), 64.0);

    vec3 lighting = light.ambient + diff * light.diffuse + spec * light.specular;
    fColor = vec4(value * lighting, 1.f);
}
)glsl"