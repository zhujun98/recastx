R"glsl(
#version 330

layout(location = 0) in vec3 vPos;

uniform mat4 mvp;

out vec3 texCoords;

void main() {
	gl_Position = mvp * vec4(vPos, 1.);
	texCoords = vPos + vec3(0.5);
}

)glsl"