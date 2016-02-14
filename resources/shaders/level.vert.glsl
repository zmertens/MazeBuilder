#version 450 core

struct Light {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec4 position; // position.w = 1 is a directional light
};

// attribute data
layout (location = 0) in vec3 aVertexPosition;
layout (location = 1) in vec2 aVertexTexCoord;
layout (location = 2) in vec3 aVertexNormal;

out VS_OUT
{
	vec3 vEyePosition;
	vec3 vEyeNormal;
	vec2 vTexCoord0;
	vec3 vLightDir;
} vs_out;

//uniform float uAtlasRows = 8.0f;
//uniform vec2 uTexOffset0;
uniform Light uLight;
uniform mat4 uModelViewMatrix;
uniform mat4 uProjMatrix;

void main(void)
{
	vs_out.vTexCoord0 = aVertexTexCoord;//(aVertexTexCoord / uAtlasRows) + uTexOffset0;

	vec4 mvPosition = uModelViewMatrix * vec4(aVertexPosition, 1.0);

	vs_out.vEyePosition = mvPosition.xyz;
	vs_out.vEyeNormal = normalize(mat3(uModelViewMatrix) * aVertexNormal);

	if (uLight.position.w == 0.0)
		vs_out.vLightDir = normalize(uLight.position.xyz - vs_out.vEyePosition);
	else // directional
		vs_out.vLightDir = normalize(uLight.position.xyz);

	gl_Position = uProjMatrix * mvPosition;
}
