#version 330 core

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
    vec4 texColor = texture(sampler, fragment_uv);
    vec3 color = texColor.rgb;

    // Discard transparent pixels or magenta color key FIRST (before any other checks)
    // Relaxed thresholds to handle texture filtering/mipmapping artifacts
    bool is_magenta = (color.r > 0.7 && color.g < 0.3 && color.b > 0.7 &&
                      abs(color.r - color.b) < 0.3 && color.g < min(color.r, color.b) * 0.5);
    if (texColor.a < 0.5 || is_magenta) {
        discard;
    }

    // Detect maze texture: fragment_light == 1.0 AND fragment_ao == 1.0 (from uv.z=0.0)
    // This must come after transparency check but before cloud check
    // Render it without any lighting, AO, or fog effects
    if (fragment_light >= 0.99 && fragment_ao >= 0.99) {
        // Maze texture detected - render at full brightness without lighting/fog
        fragColor = vec4(color, 1.0);
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
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
    // Apply fog with sky blue color that matches the background
    vec3 fog_color = vec3(0.53, 0.81, 0.92) * (daylight * 0.4 + 0.6);
    color = mix(color, fog_color, fog_factor);
    fragColor = vec4(color, 1.0);

    float brightness = dot(fragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        brightColor = vec4(fragColor.rgb, 1.0);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
