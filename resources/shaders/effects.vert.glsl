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

uniform Effect uEffect;
uniform float uTime = 0.0f;

out vec2 vTexCoords;

void main()
{
	const vec4[4] vertices = vec4[4](vec4(-1.0, -1.0, 0.0, 0.0),
									 vec4( 1.0, -1.0, 1.0, 0.0),
									 vec4(-1.0,  1.0, 0.0, 1.0),
									 vec4( 1.0,  1.0, 1.0, 1.0));

	vTexCoords = vertices[gl_VertexID].zw;
	gl_Position = vec4(vertices[gl_VertexID].xy, 0.0f, 1.0f);

	if (uEffect.type == BLUR)
	{
		float strength = 4.89;
		gl_Position.x += cos(uTime * 25.0) * strength;
		gl_Position.y += cos(uTime * 35.0) * strength;
	}
}
