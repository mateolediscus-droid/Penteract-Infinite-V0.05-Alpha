#version 330 core

in vec2 fsUV;

out vec4 color;

uniform sampler2D texSampler;
uniform vec4 inColor;

void main()
{
	color = texture(texSampler, fsUV) * inColor;
	//color = inColor;
}