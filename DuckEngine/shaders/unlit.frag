#version 460 core
out vec4 FragColor;

in vec2 vTexCoord;

struct Material
{
    vec3 baseColor;
    sampler2D baseTexture;

    vec3 emissiveColor;
    sampler2D emissiveTexture;

    float opacity;

    float alphaCutoff;
};

uniform Material material;

void main()
{
    vec3 color = vec3(0.0);

    vec4 baseTex = texture(material.baseTexture, vTexCoord);

    float finalAlpha = material.opacity * baseTex.a;
    if(finalAlpha <= material.alphaCutoff) discard;

    vec3 base = material.baseColor * baseTex.rgb;
    vec3 emissive = material.emissiveColor * texture(material.emissiveTexture, vTexCoord).rgb;

    color += base;
    color += emissive;

    FragColor = vec4(color, finalAlpha);
}