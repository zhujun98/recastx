R"glsl(

#version 330

layout(location = 0) in vec3 vPos;

uniform mat4 mvp;
uniform mat4 mvpLightSpace;

out vec3 texCoords;
out vec4 shadowCoords;

void main() {
	shadowCoords = mvpLightSpace * vec4(vPos, 1);
	gl_Position = mvp * vec4(vPos, 1);
	texCoords = vPos + vec3(0.5f);
}

)glsl"