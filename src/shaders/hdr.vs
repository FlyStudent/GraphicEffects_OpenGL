// Attributes
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aUV;

// shader output
out vec2 oUV;

void main()
{
    oUV = aUV;
    gl_Position = vec4(aPosition, 1.0);
}