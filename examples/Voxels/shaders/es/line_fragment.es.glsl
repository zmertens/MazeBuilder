#version 300 es
precision mediump float;


in vec3 our_color;
out vec4 fragColor;

void main() {
    fragColor = vec4(1.0 - our_color, 1.0);
}
