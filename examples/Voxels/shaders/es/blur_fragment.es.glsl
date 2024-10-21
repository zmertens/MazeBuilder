#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;

uniform bool horizontal;
uniform float weight[5];

void main()
{             
     // Gets size of single texel
     vec2 tex_offset = 1.0 / vec2(textureSize(image, 0));
     vec3 result = texture(image, TexCoords).rgb * weight[0];
     if(horizontal)
     {
         for(int i = 1; i < 5; ++i)
         {
            result += texture(image, TexCoords + vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * float(i), 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 5; ++i)
         {
             result += texture(image, TexCoords + vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
             result += texture(image, TexCoords - vec2(0.0, tex_offset.y * float(i))).rgb * weight[i];
         }
     }
     FragColor = vec4(result, 1.0);
}