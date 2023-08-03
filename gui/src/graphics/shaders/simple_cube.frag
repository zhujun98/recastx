R"glsl(
#version 330

in vec3 faceColor;

out vec4 fColor;

void main() {
    fColor = vec4(faceColor, 1.0f);
}
)glsl"