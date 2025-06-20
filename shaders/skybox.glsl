#version 450

#ifdef VERTEX

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 morm;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 texCoords;

layout(binding = 0) uniform _{
    mat4 _view;
    mat4 _proj;
};

void main() {
    texCoords = aPos;
    gl_Position = _proj * _view * vec4(aPos, 1.0);
    gl_Position = gl_Position.xyww;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 texCoords; 
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube Skybox;

void main() {
    outColor = texture(Skybox, texCoords);
}

#endif