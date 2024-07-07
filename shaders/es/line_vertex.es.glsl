#version 300 es
precision mediump float;


uniform mat4 matrix;

layout(location=0) in vec4 position;

void main() {
    gl_Position = matrix * position;
}
