#version 300 es
precision mediump float;

out vec4 FragColor;

uniform sampler2D sampler;
uniform float timer;

in vec2 fragment_uv;

void main()
{
    // Sky texture is a horizontal strip - timer provides time-of-day animation
    vec2 uv = vec2(timer, fragment_uv.t);
    FragColor = texture(sampler, uv);
}
