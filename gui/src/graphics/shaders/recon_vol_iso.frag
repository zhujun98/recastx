R"glsl(
#version 330

in vec3 fPos;
in vec3 outNorm;

out vec4 fColor;

struct Light {
    bool isEnabled;
    vec3 pos;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float alpha;
    float shininess;
};

uniform sampler1D colormap;
uniform sampler3D volumeData;

uniform vec3 viewPos;
uniform Light light;

uniform Material material;

void main() {
    vec3 lightDir = normalize(light.pos - fPos);
    float diff = max(dot(lightDir, outNorm), 0.0);

    vec3 viewDir = normalize(viewPos - fPos);
    vec3 reflectDir = reflect(-lightDir, outNorm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(outNorm, halfwayDir), 0.0), material.shininess);

    fColor = vec4(light.ambient * material.ambient
                  + diff * light.diffuse * material.diffuse
                  + spec * light.specular * material.specular, material.alpha);
}
)glsl"