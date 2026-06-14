#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;

out vec2 fragment_uv;

void main() {
    fragment_uv = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
