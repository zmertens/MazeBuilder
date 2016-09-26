#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

out vec2 vTexCoord;

uniform float uHalfSize;   // Half the width of the quad
uniform mat4 uProjMatrix;
uniform float uAtlasRows;
uniform vec2 uTexOffset0;

void main()
{
	gl_Position = uProjMatrix * (vec4(-uHalfSize, -uHalfSize, 0.0, 0.0) + gl_in[0].gl_Position);
	vTexCoord = (vec2(1.0, 1.0) / uAtlasRows) + uTexOffset0;
    EmitVertex();

	gl_Position = uProjMatrix * (vec4(uHalfSize, -uHalfSize, 0.0, 0.0) + gl_in[0].gl_Position);
	vTexCoord = (vec2(0.0, 1.0) / uAtlasRows) + uTexOffset0;
    EmitVertex();

	gl_Position = uProjMatrix * (vec4(-uHalfSize, uHalfSize, 0.0, 0.0) + gl_in[0].gl_Position);
	vTexCoord = (vec2(1.0, 0.0) / uAtlasRows) + uTexOffset0;
    EmitVertex();

	gl_Position = uProjMatrix * (vec4(uHalfSize, uHalfSize, 0.0, 0.0) + gl_in[0].gl_Position);
	vTexCoord = (vec2(0.0, 0.0) / uAtlasRows) + uTexOffset0;
    EmitVertex();

    EndPrimitive();
}
