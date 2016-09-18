#version 450 core

#define NONE 0
#define GRAYSCALE 1
#define INVERSION 2
#define EDGE 3
#define BLUR 4
#define SHARPEN 5

struct Effect {
    int type;
};

in vec2 vTexCoords;

layout (location = 0) out vec4 FragColor;

layout (binding = 2) uniform sampler2D uTexture2D;

uniform Effect uEffect;
// uniform initialization requires GL 4.3
uniform float uEdgeKernel[9] = {
	1.0f,  1.0f, 1.0f,
	1.0f, -8.0f, 1.0f,
	1.0f,  1.0f, 1.0f};

uniform float uBlurKernel[9] = {
	0.0625f, 0.125f, 0.0625f,
	0.125f, 0.25f, 0.125f,
	0.0625f, 0.125f, 0.0625f};

uniform float uSharpenKernel[9] = {
	-1.0f, -1.0f, -1.0f,
	-1.0f,  9.0f, -1.0f,
	-1.0f, -1.0f, -1.0f};

void main()
{
    vec4 finalShade;

	if (uEffect.type == NONE)
    {
		finalShade = texture(uTexture2D, vTexCoords);
    }
    else if (uEffect.type == GRAYSCALE)
    {
        vec3 temp = vec3(texture(uTexture2D, vTexCoords));
        float average = 0.2121 * temp.r + 0.715 * temp.g + 0.015 * temp.b;
        finalShade = vec4(average, average, average, 1.0);
    }
    else if (uEffect.type == INVERSION)
    {
        finalShade = vec4(1.0 - texture(uTexture2D, vTexCoords).rgb, 1.0);
    }
    else if (uEffect.type == BLUR)
    {
        vec3 sample_tex[9];

        sample_tex[0] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 1)));
        sample_tex[1] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 1)));
        sample_tex[2] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 1)));
        sample_tex[3] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 0)));
        sample_tex[4] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 0)));
        sample_tex[5] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 0)));
        sample_tex[6] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, -1)));
        sample_tex[7] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, -1)));
        sample_tex[8] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, -1)));

        for (int i = 0; i != 9; ++i)
        {
            finalShade += vec4(sample_tex[i] * uBlurKernel[i], 1.0f);
        }
    }
    else if (uEffect.type == EDGE)
    {
        vec3 sample_tex[9];

        sample_tex[0] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 1)));
        sample_tex[1] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 1)));
        sample_tex[2] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 1)));
        sample_tex[3] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 0)));
        sample_tex[4] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 0)));
        sample_tex[5] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 0)));
        sample_tex[6] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, -1)));
        sample_tex[7] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, -1)));
        sample_tex[8] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, -1)));

        for (int i = 0; i != 9; ++i)
        {
            finalShade += vec4(sample_tex[i] * uEdgeKernel[i], 1.0f);
        }
    }
    else if (uEffect.type == SHARPEN)
    {
        vec3 sample_tex[9];

        sample_tex[0] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 1)));
        sample_tex[1] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 1)));
        sample_tex[2] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 1)));
        sample_tex[3] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, 0)));
        sample_tex[4] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, 0)));
        sample_tex[5] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, 0)));
        sample_tex[6] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(-1, -1)));
        sample_tex[7] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(0, -1)));
        sample_tex[8] = vec3(textureOffset(uTexture2D, vTexCoords, ivec2(1, -1)));

        for (int i = 0; i != 9; ++i)
        {
            finalShade += vec4(sample_tex[i] * uSharpenKernel[i], 1.0f);
        }
    }

	FragColor = finalShade;
}
