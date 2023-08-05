R"glsl(
#version 330

in vec3 oColor;

out vec4 fColor;

void main() {
    fColor = vec4(oColor, 1.0f);
}
)glsl"