R"glsl(
#version 330 core

out vec4 fColor;

struct Material {
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

in vec3 fPos;
in vec3 fNormal;
in vec2 fTexCoord;

uniform vec3 viewPos;
uniform Material material;

vec3 computeLight(Light light, vec3 normal, vec3 viewDir, vec3 lightDir) {
    vec3 sum = vec3(0.f, 0.f, 0.f);

    sum += light.ambient * texture(material.ambient, fTexCoord).rgb;

    float diff = max(dot(normal, lightDir), 0.0);
    sum += light.diffuse * diff * texture(material.diffuse, fTexCoord).rgb;

    float spec = pow(max(dot(normal, normalize(lightDir + viewDir)), 0.f), material.shininess);
    sum += light.specular * spec * texture(material.specular, fTexCoord).rgb;

    return sum;
}

void main() {
    vec3 normal = normalize(fNormal);
    vec3 viewDir = normalize(viewPos - fPos);
    vec3 lightDir = normalize(light.position - fPos);

    fColor = vec4(computeLight(light, normal, viewDir, lightDir), 1.0);
}
)glsl"