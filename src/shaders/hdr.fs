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
	const float gamma = 2.2;
	vec3 hdrColor = texture(uScreenTexture, TexCoords).rgb;
	if (uProcess)
	{
		vec3 toneMapped = vec3(1.0) - exp(-hdrColor * uExposure);
		toneMapped = pow(toneMapped, vec3(1.0/gamma));
		FragColor = vec4(toneMapped, 1.0);
	}
	else
	{
		FragColor = vec4(pow(hdrColor, vec3(1.0/gamma)), 1.0);
	}
}