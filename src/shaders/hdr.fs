#version 330

// shader inputs
in vec2 TexCoords;

// uniforms
uniform sampler2D uScreenTexture;
uniform bool uProcess;
uniform float uExposure;

// shader ouputs
out vec4 FragColor;

void main()
{
	vec3 hdrColor = texture(uScreenTexture, TexCoords).rgb;

	vec3 toneMapped = vec3(1.0) - exp(-hdrColor * uExposure);

	FragColor = vec4(toneMapped, 1.0);
}