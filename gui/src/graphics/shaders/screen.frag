R"glsl(
#version 330

layout(location = 0) out vec4 fColor;

in vec2 texCoords;

uniform sampler2D screenTexture;

void main() {
	fColor = texture(screenTexture, texCoords);
}

)glsl"