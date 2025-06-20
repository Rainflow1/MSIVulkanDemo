#version 450

#ifdef VERTEX

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 normal;
layout(location = 1) out vec3 pos;
layout(location = 2) out vec2 texCoords;


layout(binding = 0) uniform _{
    mat4 _model;
    mat4 _view;
    mat4 _proj;
};


void main() {
    gl_Position = _proj * _view * _model * vec4(inPosition, 1.0);
    pos = vec3(model * vec4(inPosition, 1.0));
    normal = mat3(transpose(inverse(model))) * inNormal;
    texCoords = inTexCoords;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 fragPos; 
layout(location = 2) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform mat{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

layout(binding = 2) uniform _{
    //vec3 lightcolor;
    vec3 lightPos;
    vec3 _viewPos;
};

void main() {

    const vec3 lightColor = vec3(1.0, 0.3, 0.3);

    vec3 ambient = material.ambient * lightColor;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = (diff * material.diffuse) * lightColor;

    vec3 viewDir = normalize(_viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = lightColor * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    outColor = vec4(result, 1.0);
}

#endif