#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in int gsChars[];

out vec2 fsUV;

uniform mat4 MV;
uniform mat4 P;
uniform ivec2 charSize;

void main()
{
	float charOffsetX = gl_PrimitiveIDIn * charSize.x;

	vec2 charStartPos = vec2(charOffsetX, 0);
	vec2 charEndPos = vec2(charOffsetX + charSize.x, charSize.y);

	vec2 charStartUV = vec2(float(gsChars[0]) / 128, 0);
	vec2 charEndUV = vec2(float(gsChars[0] + 1) / 128, 1);

	// create the triangles
	fsUV = vec2(charStartUV.x, charStartUV.y);
	gl_Position = P * MV * vec4(charStartPos.x, charStartPos.y, 0, 1);
	EmitVertex();

	fsUV = vec2(charEndUV.x, charStartUV.y);
	gl_Position = P * MV * vec4(charEndPos.x, charStartPos.y, 0, 1);
	EmitVertex();

	fsUV = vec2(charStartUV.x, charEndUV.y);
	gl_Position = P * MV * vec4(charStartPos.x, charEndPos.y, 0, 1);
	EmitVertex();

	fsUV = charEndUV;
	gl_Position = P * MV * vec4(charEndPos.x, charEndPos.y, 0, 1);
	EmitVertex();
}