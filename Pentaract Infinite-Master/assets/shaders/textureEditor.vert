#version 450 core
layout(location=0) in vec3 p;
layout(location=1) in int side;
uniform mat4 mvp;
out vec3 position;
flat out int face;
void main(){position=p;face=side;gl_Position=mvp*vec4(p,1);}
