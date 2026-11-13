#version 450 core

in vec2 uv;

out vec4 color;

uniform sampler2D texSampler;
uniform vec4 clip;
//uniform vec4 inColor;

void main()
{
	color = texture(texSampler, mix(clip.xy, clip.xy + clip.zw, uv));
	//color = texture(texSampler, uv) * inColor;
}