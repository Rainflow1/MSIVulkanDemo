#version 450

#ifdef VERTEX

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 inColor;


layout(binding = 0) uniform _{
    mat4 _model;
    mat4 _view;
    mat4 _proj;
};

layout(binding = 1) uniform mat{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

layout(binding = 2) uniform ya{
    //vec3 lightcolor;
    vec3 lightPos;
    vec3 _viewPos;
};


void main() {
    gl_Position = _proj * _view * _model * vec4(inPosition, 1.0);

    vec3 fragPos = vec3(_model * vec4(inPosition, 1.0));
    vec3 normal = mat3(transpose(inverse(_model))) * inNormal;

    const vec3 lightColor = vec3(1.0, 1, 1);

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

    inColor = result;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;


void main() {
    outColor = vec4(inColor, 1.0);
}

#endif