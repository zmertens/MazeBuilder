#version 300 es
precision mediump float;

uniform sampler2D scene;
uniform sampler2D bloom_blur;
uniform float bloom_strength;

in vec2 fragment_uv;
out vec4 fragColor;

void main() {
    vec3 scene_color = texture(scene, fragment_uv).rgb;
    vec3 bloom_color = texture(bloom_blur, fragment_uv).rgb;
    vec3 result = scene_color + bloom_color * bloom_strength;
    fragColor = vec4(result, 1.0);
}
