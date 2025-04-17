#version 450

#ifdef VERTEX

layout(location = 0) out vec3 fragColor;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(binding = 0) uniform M{mat4 model;}m;
layout(binding = 1) uniform V{mat4 view;}v;
layout(binding = 2) uniform P{mat4 proj;}p;

void main() {
    gl_Position = p.proj * v.view * m.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}

#endif