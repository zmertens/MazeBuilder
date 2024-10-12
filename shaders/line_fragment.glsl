#version 330 core

in vec3 our_color;

out vec4 frag_color;

void main() {
	const float gamma = 2.2;
	vec3 next_color = pow(our_color, vec3(1.0 / gamma));
    frag_color = vec4(1.0 - next_color, 1.0);
}
