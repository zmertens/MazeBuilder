#version 430 core

in vec2 vTexCoord;

uniform sampler2D uTexture2D;

layout (location = 0) out vec4 FragColor;

void main()
{
	vec4 texSample = texture(uTexture2D, vTexCoord);

	if (texSample.a < 0.2)
		discard;

	FragColor = texSample;
}

