R"glsl(
#version 330

in vec3 fPos;
in vec3 volumeCoord;

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

uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

uniform float alpha;
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
        value = remapMinMax(texture(volumeData, volumeCoord).x, minValue, maxValue);
    }

    vec4 color = vec4(texture(colormap, value).xyz, alpha * value);

    if (light.isEnabled) {
        float diffuse = 0.f;
        float specular = 0.f;

        vec3 viewDir = normalize(viewPos - fPos);

        vec3 rgb = light.ambient * color.rgb;
        rgb += light.diffuse * color.rgb * diffuse;
        rgb += light.specular * color.rgb * specular;
        fColor = vec4(rgb, color.a);
    } else {
        fColor = color;
    }
}
)glsl"