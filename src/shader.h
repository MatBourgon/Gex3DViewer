#pragma once

struct shaderinfo_t
{
	const char* szVertexFileName;
	const char* szFragFileName;
};
bool LoadShader(unsigned int& program, shaderinfo_t info);