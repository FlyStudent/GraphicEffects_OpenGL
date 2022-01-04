#version 330 core

// shader inputs
in vec2 TexCoords;

// uniforms
uniform sampler2D hdrBuffer;

// shader ouputs
out vec4 oColor;

void main()
{
	vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
	oColor = vec4(hdrColor, 1.0);
}