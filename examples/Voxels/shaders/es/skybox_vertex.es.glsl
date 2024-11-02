#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 matrix;

void main()
{
    TexCoords = aPos;
    vec4 pos = matrix * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
} 