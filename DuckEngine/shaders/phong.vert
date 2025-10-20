#version 460 core
layout (location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;


layout(std140, binding = 0) uniform Camera
{
    mat4 view;
    mat4 projection;
};

uniform mat4 model;

void main()
{
    vFragPos = vec3(model * vec4(aPos, 1.0));
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vTexCoord = aTexCoord;

	gl_Position = projection * view * model * vec4(aPos, 1.0);
}