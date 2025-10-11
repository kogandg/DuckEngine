#version 460 core

struct Material 
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct Light
{
    vec4 position; //xyz = position, w=1 for point/spot, 0 for direction
    vec4 direction; //xyz = direction, w = cutOff for spot

    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec4 params; //x = constant, y = linear, z = quadratic, w = outerCutOff

    ivec4 type;
    //int type; //0 = directional, 1 = point, 2 = spot
    //int _pad[3];
};

#define MAX_LIGHTS 32
layout(std140, binding = 1) uniform Lights
{
    ivec4 numLights;
    //int numLights;
    //int _pad[3];
    Light lights[MAX_LIGHTS];
};

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform Material material;


uniform vec3 viewPos;

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
//    vec3 norm = normalize(Normal);
//    vec3 viewDir = normalize(viewPos - FragPos);
//
//    vec3 result = vec3(0.0);
//
//    for(int i = 0; i < numLights[0]; i++)
//    {
//        switch(lights[i].type[0])
//        {
//        case 0:
//            result += CalcDirLight(lights[i], norm, viewDir);
//            break;
//        case 1:
//            result += CalcPointLight(lights[i], norm, FragPos, viewDir);
//            break;
//        case 2:
//            result += CalcSpotLight(lights[i], norm, FragPos, viewDir);
//            break;
//        }
//    }
//
//    FragColor = vec4(result, 1.0);

    FragColor = vec4(1.0);
}

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction.xyz);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = (material.shininess > 0.0) ? pow(specBase, material.shininess) : 1.0;//pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    //combine results
    vec3 ambient = light.ambient.rgb * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse.rgb * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular.rgb * spec * vec3(texture(material.specular, TexCoords));

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
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
    float attenuation = 1.0 / (light.params.x + light.params.y * distance + light.params.z * (distance * distance));   
    //combine results
    vec3 ambient = light.ambient.rgb * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse.rgb * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular.rgb * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    float cutOff = light.direction.w;
    float outerCutOff = light.params.w;
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    //diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    //specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specBase = max(dot(viewDir, reflectDir), 0.0);
    float spec = (material.shininess > 0.0) ? pow(specBase, material.shininess) : 1.0;
    //spotlight edges
    float theta = dot(lightDir, normalize(-light.direction.xyz));
    float epsilon = cutOff - outerCutOff;
    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);
    //attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.params.x + light.params.y * distance + light.params.z * (distance * distance));  
    //combine results
    vec3 ambient = light.ambient.rgb * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse.rgb * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular.rgb * spec * vec3(texture(material.specular, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}