uniform sampler2D texture;
uniform float threshold;
uniform float intensity;

void main()
{
    vec4 c = texture2D(texture, gl_TexCoord[0].xy) * gl_Color;
    float luma = dot(c.rgb, vec3(0.2126, 0.7152, 0.0722));
    float bright = max((luma - threshold) / max(0.0001, 1.0 - threshold), 0.0);
    gl_FragColor = vec4(c.rgb * bright * intensity, c.a * bright);
}
