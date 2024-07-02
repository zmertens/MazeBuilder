#version 300 es
precision mediump float;

uniform sampler2D sampler;
uniform float timer;

in vec2 fragment_uv;

out vec4 fragColor;

void main() {
    vec2 uv = vec2(timer, fragment_uv.t);
    fragColor = vec4(texture(sampler, uv).rgb, 1.0);
}
