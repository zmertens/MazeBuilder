#version 450 core

#define INV_GAMMA 0.4545454545

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec4 position; // position.w = 1 is a directional light
};

in VS_OUT
{
	vec3 vEyePosition;
	vec3 vEyeNormal;
	vec2 vTexCoord0;
	vec3 vLightDir;
} fs_in;

layout (location = 0) out vec4 FragColor;

layout (binding = 0) uniform sampler2D uTexture2D;

uniform Material uMaterial;
uniform Light uLight;

// Phong shading algorithm:
// = Ma * La +
// Md * Ld * max(dot(L, N), 0) +
// Ms * Ls * pow(max(dot(V, R), 0), alpha)
vec3 phongShading(Light light, vec3 ambMat, vec3 diffMat, vec3 specMat, float shininessMat)
{
	vec3 normal;
	if (!gl_FrontFacing)
		normal = -fs_in.vEyeNormal;

	vec3 viewDir = normalize(-fs_in.vEyePosition);
	vec3 reflectDir = reflect(-fs_in.vLightDir, normal);

	vec3 ambient = ambMat * light.ambient;

	float LdotN = max(dot(fs_in.vLightDir, fs_in.vEyeNormal), 0.0);
	vec3 diffuse = diffMat * light.diffuse * LdotN;
	vec3 spec;

	if (LdotN > 0.0)
	{
		spec = specMat * light.specular * pow(max(dot(reflectDir, viewDir), 0.0), shininessMat);
	}

	return ambient + diffuse + spec;
}

void main(void)
{
	vec4 texSample = texture(uTexture2D, fs_in.vTexCoord0);

	if (texSample.a < 0.25)
		discard;

	vec3 shade = phongShading(uLight, uMaterial.ambient,
							  uMaterial.diffuse, uMaterial.specular,
							  uMaterial.shininess);

	FragColor = texSample * vec4(shade, 1.0);
}
