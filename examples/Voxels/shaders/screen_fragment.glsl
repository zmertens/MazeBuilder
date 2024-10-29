#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform float exposure;

uniform bool do_bloom;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;      

	if (do_bloom) {
		vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
		// Additive blending
    	hdrColor += bloomColor;
	}

    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
} 
