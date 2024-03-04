R"glsl(
#version 330

layout (location = 0) in vec2 vPos;

uniform vec4 center;
uniform float xScale;

void main() {
    gl_Position = vec4(xScale * vPos.x + center.x, vPos.y + center.y, 0.0, 1.0);
}

)glsl"