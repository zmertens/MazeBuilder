#version 150

in vec2 tex_coord;

void main()
{
    vec2 centered = tex_coord * 2.0 - vec2(1.0, 1.0);
    float dist_sq = dot(centered, centered);
    if (dist_sq > 1.0)
    {
        discard;
    }

    float alpha = smoothstep(1.0, 0.0, dist_sq);
    vec3 glow = vec3(1.0, 0.87, 0.24);
    gl_FragColor = vec4(glow, alpha);
}
