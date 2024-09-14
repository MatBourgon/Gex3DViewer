#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUV;
//layout (location = 2) in vec3 aNormal;

uniform mat4 uCamera;

out vec4 vCol;
out vec2 vUV;

void main()
{
    gl_Position = (uCamera * vec4(aPos.xyz, 1.0));
    //vec3 cPos = vec3(uCamera[0][3], uCamera[1][3], uCamera[2][3]);
    //cPos = cPos - gl_Position.xyz;
    vCol = aColor;
    vUV = aUV;
}