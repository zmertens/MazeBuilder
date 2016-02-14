#version 430 core

uniform mat4 uModelViewMatrix;

void main()
{
	gl_Position = uModelViewMatrix * vec4(1.0);
}

