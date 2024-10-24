#version 330 core

uniform sampler2D sampler;
uniform bool is_sign;

in vec2 fragment_uv;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

void main() {
    vec4 color = vec4(texture(sampler, fragment_uv).rgb, 1.0);
    if (is_sign) {
        if (color == vec4(1.0)) {
            discard;
        }
    }
    else {
        color.a = max(color.a, 0.4);
    }
    fragColor = color;

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        brightColor = vec4(fragColor.rgb, 1.0);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
