#version 330 core

in vec3 vTexSkyboxCoord;

layout (location = 0) out vec4 FragColor;

uniform samplerCube uSkybox;

void main(void)
{
	FragColor = texture(uSkybox, vTexSkyboxCoord);
}

