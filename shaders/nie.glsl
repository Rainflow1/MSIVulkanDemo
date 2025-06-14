#version 450

#ifdef VERTEX

layout(location = 0) out vec3 fragColor;

layout(location = 1) in vec3 inPosition;
//layout(location = 1) in vec3 inColor;
layout(location = 0) in vec3 inNormal;

layout(binding = 0) uniform _{
    mat4 model;
    mat4 view;
    mat4 proj;
};
/*
layout(set=1, binding = 0) uniform M{mat4 _model;};
layout(binding = 1) uniform V{mat4 _view;};
layout(binding = 2) uniform P{mat4 _proj;};
layout(binding = 3) uniform E{
    vec3 color;
};
*/
void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    fragColor = vec3(1.0, 1.0, 0.0);
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(fragColor, 1.0);
}

#endif