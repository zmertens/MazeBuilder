#version 300 es
precision mediump float;

uniform sampler2D sampler;
uniform float timer;
uniform float daylight;
uniform bool is_ortho;

in vec2 fragment_uv;
in float fragment_ao;
in float fragment_light;
in float fog_factor;
in float fog_height;
in float diffuse;

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

const float pi = 3.14159265;

void main() {
    vec3 color = texture(sampler, fragment_uv).rgb;
    if (color == vec3(1.0, 0.0, 1.0)) {
        discard;
    }
    bool cloud = color == vec3(1.0, 1.0, 1.0);
    if (cloud && is_ortho) {
        discard;
    }
    float df = cloud ? diffuse * 1.2 : diffuse;
    float ao = cloud ? fragment_ao * 1.2 : fragment_ao;

    ao += fragment_light;
    df += fragment_light;
    float value = daylight + fragment_light;
    vec3 light_color = vec3(value * 0.3 + 0.2);
    vec3 ambient = vec3(value * 0.3 + 0.2);
    vec3 light = ambient + light_color * df;
    color *= light * ao;
    vec3 fog_mix = vec3(timer, fog_height, 0.0);
    color = mix(color, vec3(0.0), fog_mix.y * fog_factor);
    fragColor = vec4(color, 1.0);
    
    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        brightColor = vec4(fragColor.rgb, 1.0);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
