#version 460 core

#define MAX_LIGHTS 32

out vec4 FragColor;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

struct Light
{
    vec4 position;   // xyz = position, w = 1 for point/spot, 0 for direction
    vec4 direction;  // xyz = direction, w = innerCutOff (for spot)
    
    vec3 ambient;    float _padAmbient;
    vec3 diffuse;    float _padDiffuse;
    vec3 specular;   float _padSpecular;
    
    // individual attenuation/cutoff floats (pack into one vec4 slot)
    float constant;
    float linear;
    float quadratic;
    float outerCutoff;
    
    int type;        // 0 = directional, 1 = point, 2 = spot
    int _pad0;
    int _pad1;
    int _pad2;
};

layout(std140, binding = 1) uniform Lights
{
    ivec4 numLights; // numLights.x = count, yzw = padding
    Light lights[MAX_LIGHTS];
};

struct Material
{
    vec3 ambientColor;

    vec3 baseColor;
    sampler2D baseTexture;

    vec3 specularColor;
    sampler2D specularTexture;
    float shininess;

    vec3 emissiveColor;
    sampler2D emissiveTexture;

    float opacity;
};

uniform Material material;

uniform vec3 viewPos;

//uniform vec3 ambientColor = vec3(1.0);
//
//uniform vec3 baseColor = vec3(1.0);
//uniform sampler2D baseTexture;
//
//uniform vec3 specularColor = vec3(1.0);
//uniform sampler2D specularTexture;
//uniform float shininess = 32.0;
//
//uniform float opacity = 1.0;
//
//uniform vec3 emissiveColor = vec3(0.0);
//uniform sampler2D emissiveTexture;

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular);
vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular);
vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular);

void main()
{
    vec3 result = vec3(0.0);


    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - vFragPos);

    vec3 baseDiffuse = material.baseColor * vec3(texture(material.baseTexture, vTexCoord));
    vec3 baseSpecular = material.specularColor * vec3(texture(material.specularTexture, vTexCoord));
    vec3 baseEmissive = material.emissiveColor * texture(material.emissiveTexture, vTexCoord).rgb;

    for(int i = 0; i < numLights[0]; i++)
    {
        switch(lights[i].type)
        {
        case 0:
            result += CalcDirLight(lights[i], normal, viewDir, baseDiffuse, baseSpecular);
            break;
        case 1:
            result += CalcPointLight(lights[i], normal, vFragPos, viewDir, baseDiffuse, baseSpecular);
            break;
        case 2:
            result += CalcSpotLight(lights[i], normal, vFragPos, viewDir, baseDiffuse, baseSpecular);
            break;
        }
    }

    result += baseEmissive;
    FragColor = vec4(result, material.opacity);
}

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular)
{
    vec3 lightDir = normalize(-light.direction.xyz);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = (material.shininess > 0.0) ? pow(specBase, material.shininess) : 1.0;//pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //combine results
    vec3 ambient = light.ambient.rgb * material.ambientColor * baseDiffuse;
    vec3 diffuse = light.diffuse.rgb * diff * baseDiffuse;
    vec3 specular = light.specular.rgb * spec * baseSpecular;

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular)
{
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = (material.shininess > 0.0) ? pow(specBase, material.shininess) : 1.0;
    //attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    //combine results
    vec3 ambient = light.ambient.rgb * material.ambientColor * baseDiffuse;
    vec3 diffuse = light.diffuse.rgb * diff * baseDiffuse;
    vec3 specular = light.specular.rgb * spec * baseSpecular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseDiffuse, vec3 baseSpecular)
{
    float cutOff = light.direction.w;
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = (material.shininess > 0.0) ? pow(specBase, material.shininess) : 1.0;
    //spotlight edges
    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon = cutOff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
    //attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    //combine results
    vec3 ambient = light.ambient.rgb * material.ambientColor * baseDiffuse;
    vec3 diffuse = light.diffuse.rgb * diff * baseDiffuse;
    vec3 specular = light.specular.rgb * spec * baseSpecular;
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}