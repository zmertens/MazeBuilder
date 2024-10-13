#version 330 core

uniform mat4 matrix;

layout(location = 0) in vec4 position;

out vec3 our_color;

void main() {
    gl_Position = matrix * position;
    our_color = vec3(1.0, 0.0, 0.0);
}
