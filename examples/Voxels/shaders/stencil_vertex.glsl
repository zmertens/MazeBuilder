#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 matrix;
uniform float scale_factor;

void main()
{
    // Scale up the object slightly for outline effect
    vec3 scaled_pos = position * scale_factor;
    gl_Position = matrix * vec4(scaled_pos, 1.0);
}

