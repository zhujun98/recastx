R"glsl(
#version 330

layout(location = 0) in vec3 vPos;

uniform mat4 mvp;
uniform mat4 mvpShadow;

out vec3 texCoords;
out vec4 shadowCoords;

void main() {
	shadowCoords = mvpShadow * vec4(2.f * vPos, 1.0f);
	gl_Position = mvp * vec4(2.f * vPos, 1.0f);
	texCoords = vPos + vec3(0.5f);
}

)glsl"