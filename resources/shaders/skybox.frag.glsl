#version 450 core

in vec3 vTexSkyboxCoord;

layout (location = 0) out vec4 FragColor;

layout (binding = 0) uniform samplerCube uSkybox;

void main(void)
{
	FragColor = texture(uSkybox, vTexSkyboxCoord);
}

