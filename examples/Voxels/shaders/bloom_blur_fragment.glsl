#version 330 core

uniform sampler2D image;
uniform bool horizontal;

in vec2 fragment_uv;
out vec4 fragColor;

// 9-tap Gaussian weights (normalised).
// Kernel: [0.0625, 0.25, 0.375, 0.25, 0.0625] × [same] spread over 5 taps each side.
const float weight[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main() {
    vec2 tex_offset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, fragment_uv).rgb * weight[0];

    if (horizontal) {
        for (int i = 1; i < 5; ++i) {
            result += texture(image, fragment_uv + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, fragment_uv - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
        }
    } else {
        for (int i = 1; i < 5; ++i) {
            result += texture(image, fragment_uv + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
            result += texture(image, fragment_uv - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
        }
    }

    fragColor = vec4(result, 1.0);
}
