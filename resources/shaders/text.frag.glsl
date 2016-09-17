#version 300 es
precision mediump float;

in vec2 vTexCoords;

out vec4 FragColor;

uniform sampler2D uTexture2D;
uniform vec3 uColor;

void main()
{
    vec4 sampled;
    sampled = vec4(1.0, 1.0, 1.0, texture(uTexture2D, vTexCoords).r);

    FragColor = vec4(uColor, 1.0) * sampled;
}
