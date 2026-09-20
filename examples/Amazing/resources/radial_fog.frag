uniform sampler2D texture;
uniform vec2 focus_center;
uniform float clear_radius;
uniform float feather;
uniform float blur_radius;
uniform float fog_darkness;

void main()
{
    vec2 uv = gl_TexCoord[0].xy;

    vec2 offx = vec2(blur_radius, 0.0);
    vec2 offy = vec2(0.0, blur_radius);

    vec4 sharp = texture2D(texture, uv);
    vec4 blurred = texture2D(texture, uv) * 4.0 +
                   texture2D(texture, uv - offx) * 2.0 +
                   texture2D(texture, uv + offx) * 2.0 +
                   texture2D(texture, uv - offy) * 2.0 +
                   texture2D(texture, uv + offy) * 2.0 +
                   texture2D(texture, uv - offx - offy) * 1.0 +
                   texture2D(texture, uv - offx + offy) * 1.0 +
                   texture2D(texture, uv + offx - offy) * 1.0 +
                   texture2D(texture, uv + offx + offy) * 1.0;
    blurred /= 16.0;

    float d = distance(uv, focus_center);
    float focus = 1.0 - smoothstep(clear_radius, clear_radius + feather, d);

    vec4 color = mix(blurred, sharp, focus);
    float outer = smoothstep(clear_radius * 0.75, clear_radius + feather * 1.5, d);
    color.rgb *= 1.0 - fog_darkness * outer;

    gl_FragColor = gl_Color * color;
}
