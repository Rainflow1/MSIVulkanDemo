#version 450

#ifdef VERTEX

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 fragPos; 

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
*/
void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    fragColor = vec3(1.0, 1.0, 0.0);
    fragPos = vec3(model * vec4(inPosition, 1.0));
    normal = mat3(transpose(inverse(model))) * inNormal;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 fragPos; 

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform _{
    vec3 color;
    vec3 lightPos;
};

void main() {

    const vec3 lightColor = vec3(1.0, 1.0, 0.3);

    vec3 ambient = 0.1 * lightColor;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * color;
    outColor = vec4(result, 1.0);
}

#endif