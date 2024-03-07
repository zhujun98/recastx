R"glsl(
#version 330

layout(location = 0) out vec4 fColor;

in vec2 texCoords;

uniform sampler2D screenTexture;

void main() {
	vec4 sample = texture(screenTexture, texCoords);
    fColor = vec4(sample.rgb * sample.a, sample.a);}

)glsl"