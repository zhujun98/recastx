R"glsl(
#version 330

layout(location = 0) in vec3 vPos;
layout(location = 1) in float vDist;

uniform mat4 mvp;

out vec3 texCoords;
out float sliceDist;

void main() {
	gl_Position = mvp * vec4(vPos, 1.);
	texCoords = vPos + vec3(0.5);
	sliceDist = vDist;
}

)glsl"