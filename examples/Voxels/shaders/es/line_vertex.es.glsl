#version 300 es
precision mediump float;

uniform mat4 matrix;

layout(location=0) in vec4 position;

out vec3 our_color;

void main() {
    our_color = vec3(0.0, 0.0, 1.0);
    gl_Position = matrix * position;
}
