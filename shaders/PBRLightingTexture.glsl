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

layout(binding = 2) uniform sampler2D albedoTex;
layout(binding = 4) uniform sampler2D normTex;
layout(binding = 5) uniform sampler2D heightTex;
layout(binding = 6) uniform sampler2D aoTex;
layout(binding = 7) uniform sampler2D metallicTex;
layout(binding = 8) uniform sampler2D roughnessTex;
layout(binding = 9) uniform samplerCube Skybox;


mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){ 
    // get edge vectors of the pixel triangle 
    vec3 dp1 = dFdx(p); 
    vec3 dp2 = dFdy(p); 
    vec2 duv1 = dFdx(uv); 
    vec2 duv2 = dFdy(uv);   
    // solve the linear system 
    vec3 dp2perp = cross(dp2, N); 
    vec3 dp1perp = cross(N, dp1); 
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;   
    // construct a scale-invariant frame 
    float invmax = inversesqrt(max(dot(T,T), dot(B,B))); 
    return mat3(T * invmax, B * invmax, N ); 
}

vec3 perturb_normal(vec3 N, vec3 V, vec2 texcoord, sampler2D tak){
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye) 
    vec3 map = texture(tak, texcoord).xyz; 
    mat3 TBN = cotangent_frame( N, -V, texcoord ); 
    return normalize( TBN * map );
}

vec2 ParallaxMapping(vec2 texCoord, vec3 viewDir){
    const float numLayers = 10, heightScale = 0.3;

    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy * heightScale; 
    vec2 deltaTexCoords = P / numLayers;

    vec2  currentTexCoords = texCoord;
    float currentDepthMapValue = texture(heightTex, currentTexCoords).r;
    
    while(currentLayerDepth < currentDepthMapValue){
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(heightTex, currentTexCoords).r;  
        currentLayerDepth += layerDepth;  
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightTex, prevTexCoords).r - currentLayerDepth + layerDepth;
    
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords; 
}

vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
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

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
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
    vec2 texCoord = ParallaxMapping(TexCoords,  V);
    vec3 N = perturb_normal(Normal, V, texCoord, normTex);
    vec3 R = reflect(-V, normalize(Normal));

    vec3 albedo = pow(texture(albedoTex, texCoord).rgb, vec3(2.2));
    float metallic = texture(metallicTex, texCoord).r;
    float roughness = texture(roughnessTex, texCoord).r;
    float AO = texture(aoTex, texCoord).r;
    vec3 metallicColor = texture(Skybox, R).rgb;

    const vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 1; i++){
        vec3 L = normalize(lightPos - FragPos);
        vec3 H = normalize(V + L);

        float dist = length(lightPos - FragPos);
        float attenuation = 1.0 / (dist * dist);
        vec3 radiance = lightColor * attenuation;

        float NDF = DistributionGGX(N, H, roughness);       
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0)  + 0.0001;
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }

    vec3 ambient = vec3(0.03) * albedo * AO * metallicColor;
    vec3 color = ambient + Lo;
    
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, 1.0);
}

#endif