#version 330 core

uniform sampler2D uParticleTex;

in float Transp;

layout ( location = 0 ) out vec4 FragColor;

void main()
{
    FragColor = texture(uParticleTex, gl_PointCoord);
    FragColor.a *= Transp;
}
