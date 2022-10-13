R"glsl(
#version 330

layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>

out vec2 aGlyphCoords;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(vertex.xy, 0.0, 1.0);
    aGlyphCoords = vertex.zw;
}
)glsl"