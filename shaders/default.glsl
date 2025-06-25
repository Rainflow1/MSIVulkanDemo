#version 450

#ifdef VERTEX

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texCoords;

layout(binding = 0) uniform _{
    mat4 _model;
    mat4 _view;
    mat4 _proj;
};

void main() {
    gl_Position = _proj * _view * _model * vec4(inPosition, 1.0);
    texCoords = inTexCoords;
    fragColor = vec3(1.0, 1.0, 0.0);
}

#endif

#ifdef FRAGMENT

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 texCoords;

layout(binding = 1) uniform sampler2D tex;
layout(binding = 2) uniform _{
    vec3 color;
};

void main() {
    outColor = texture(tex, texCoords);
}

#endif