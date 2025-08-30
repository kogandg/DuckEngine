#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

//uniform sampler2D texture1;
//uniform sampler2D texture2;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    FragColor = vec4(objectColor * lightColor, 1.0f);
    //FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
    //FragColor = texture(texture1, TexCoord);
}