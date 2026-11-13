#version 330 core

layout (location = 0) in vec2 vertex;
layout (location = 1) in vec4 color;

// remember that mat4s take up four attribute locations
layout (location = 2) in mat4 model;

out vec4 outColor;

uniform mat4 P;

void main()
{
    gl_Position = P * model * vec4(vertex, 0.0, 1.0);
    outColor = color;
}