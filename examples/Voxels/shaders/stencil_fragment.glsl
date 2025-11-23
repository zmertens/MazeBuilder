#version 330 core

out vec4 FragColor;

uniform vec3 outline_color;

void main()
{
    // Draw outline in a solid color (default: bright orange/yellow)
    FragColor = vec4(outline_color, 1.0);
}

