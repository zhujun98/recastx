R"glsl(
#version 330

in vec3 position;
uniform mat4 transformMatrix;

void main() {
    gl_Position = transformMatrix * vec4(position.x, position.y, position.z, 1.0f);
}
)glsl"