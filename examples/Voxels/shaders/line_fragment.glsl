#version 330 core

in vec3 our_color;

out vec4 frag_color;

void main() {
    frag_color = vec4(1.0 - our_color, 1.0);
}
