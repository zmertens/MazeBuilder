#version 300 es
precision mediump float;


uniform mat4 matrix;

layout(location=0) in vec4 position;
layout(location=1) in vec2 uv;

out vec2 fragment_uv;

void main() {
    gl_Position = matrix * position;
    fragment_uv = uv;
}
