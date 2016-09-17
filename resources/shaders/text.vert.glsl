#version 300 es
precision mediump float;

layout (location = 0) in vec4 aVertexPosAndTex; // <vec2 pos, vec2 tex>

out vec2 vTexCoords;

uniform mat4 uProjection;

void main()
{
    vTexCoords = aVertexPosAndTex.zw;

    gl_Position = uProjection * vec4(aVertexPosAndTex.xy, 0.0, 1.0);
}

