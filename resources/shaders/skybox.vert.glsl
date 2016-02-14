#version 450 core

out vec3 vTexSkyboxCoord;

uniform mat4 uViewMatrix;

void main(void)
{
	const vec3[4] vertices = vec3[4](vec3(-1.0, -1.0, 1.0),
									 vec3( 1.0, -1.0, 1.0),
									 vec3(-1.0,  1.0, 1.0),
									 vec3( 1.0,  1.0, 1.0));

	vTexSkyboxCoord = mat3(transpose(uViewMatrix)) * vertices[gl_VertexID];

	gl_Position = vec4(vertices[gl_VertexID], 1.0);
}

