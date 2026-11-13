#version 450 core
layout(location=0) in vec3 position;
layout(location=1) in ivec3 cell;
layout(location=2) in uvec2 material;
uniform vec3 meshEye,meshForward,meshRight,meshUp;
uniform float aspect;
out vec3 meshPosition;
flat out ivec3 meshCell;
flat out uvec2 meshMaterial;
void main() {
    vec3 delta=position-meshEye;
    vec3 camera=vec3(dot(delta,meshRight),dot(delta,meshUp),dot(delta,meshForward));
    const float nearPlane=.03,farPlane=160.;
    gl_Position=vec4(camera.x/aspect,camera.y,
        (farPlane+nearPlane)/(farPlane-nearPlane)*camera.z-2.*farPlane*nearPlane/(farPlane-nearPlane),camera.z);
    meshPosition=position;meshCell=cell;meshMaterial=material;
}
