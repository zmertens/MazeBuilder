#version 330 core

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    fragColor = texture(skybox, TexCoords);

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 0.2) {
        brightColor = vec4(fragColor.rgb, 1.0);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}