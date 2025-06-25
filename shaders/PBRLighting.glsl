#version 450

const float PI = 3.14159265359;

#ifdef VERTEX

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec3 Pos;
layout(location = 2) out vec2 TexCoords;


layout(binding = 0) uniform _{
    mat4 _model;
    mat4 _view;
    mat4 _proj;
};


void main() {
    gl_Position = _proj * _view * _model * vec4(inPosition, 1.0);
    Pos = vec3(_model * vec4(inPosition, 1.0));
    Normal = mat3(transpose(inverse(_model))) * inNormal;
    TexCoords = inTexCoords;
}

#endif

#ifdef FRAGMENT

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 FragPos; 
layout(location = 2) in vec2 TexCoords;

layout(location = 0) out vec4 outColor;

layout(binding = 3) uniform _{
    //vec3 lightcolor;
    vec3 lightPos;
    vec3 _viewPos;
};

layout(binding = 2) uniform _1{
    vec3 inAlbedo;
    float inMetallic;
    float inRoughness;
    float inReflectance;
};
layout(binding = 9) uniform samplerCube Skybox;


vec3 fresnelSchlick(float cosTheta, vec3 F0){
    float F90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.0, 1.0);
    return F0 + (F90 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
	
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}


void main() {
    vec3 V = normalize(_viewPos - FragPos);
    vec3 N = normalize(Normal);
    vec3 R = reflect(-V, N);

    vec3 metallicColor = texture(Skybox, R).rgb;

    float metallic = clamp(inMetallic,0.0,1.0);
    float roughness = clamp(inRoughness,0.0,1.0);
    float reflectance = clamp(inReflectance,0.0,1.0);
    vec3 albedo = inAlbedo;
    vec3 reflectColor = albedo * (1.0 - reflectance) + metallicColor * reflectance * (1.0 - roughness);

    const vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);

    float dist = length(lightPos - FragPos);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = lightColor * attenuation;

    //vec3 F0 = albedo * (1.0 - metallic) + metallicColor * metallic;
    vec3 F0 = mix(vec3(0.16), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kD = vec3(1.0) - F;
    kD *= 1.1 - metallic;

    float NDF = DistributionGGX(N, H, roughness);       
    float G = GeometrySmith(N, V, L, roughness);
    vec3 specular = (NDF * G * albedo) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);

    float NdotL = max(dot(N, L), 0.0);                
    vec3 Lo = (kD * reflectColor / PI + specular)  * radiance * NdotL;

    vec3 ambient = vec3(0.001) * reflectColor;

    vec3 color = ambient + Lo;
    
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}

#endif