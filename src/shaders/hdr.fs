#version 330

// shader inputs
in vec2 TexCoords;

// uniforms
uniform sampler2D uhdrBuffer;
uniform bool uProcess;
uniform float uExposure;

// shader ouputs
out vec4 oColor;

void main()
{
	vec3 hdrColor = texture(uhdrBuffer, TexCoords).rgb;

	vec3 toneMapped = vec3(1.0) - exp(-hdrColor * uExposure);

	oColor = vec4(toneMapped, 1.0);
}