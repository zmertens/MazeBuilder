// Finalize gamma correction and bloom effect

// Apply tone mapping for Bloom effect

#version 330 core
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;

uniform bool bloom;
uniform float exposure;

out vec4 FragColor;

void main()
{ 
    const float gamma = 2.2;
	vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
	vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
	
	if (bloom) {
		// Do additive blending
		hdrColor += bloomColor;
	}
	
	vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
	
	FragColor = vec4(pow(result, vec3(1.0/gamma)), 1.0);
}
