R"glsl(

#version 330

layout(location = 0) in vec3 vPos;

uniform mat4 mvpLight;

out vec3 texCoords;

void main() {
	gl_Position = mvpLight * vec4(vPos, 1.);
	texCoords = vPos + vec3(0.5);
}

)glsl"