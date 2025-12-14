#version 330 core

out vec4 FragColor;

uniform sampler2D sampler;
uniform float timer;

in vec2 fragment_uv;

void main()
{
    // Original code used timer for animated sky:
     vec2 uv = vec2(timer, fragment_uv.t);
     FragColor = texture(sampler, uv);
}
