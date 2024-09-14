#include "shader.h"

#include <cstdio>
#include <memory>
#include <glad/glad.h>

char* ReadShaderFile(const char* filepath)
{
    if (FILE* file = fopen(filepath, "rb"))
    {
        fseek(file, 0, SEEK_END);
        size_t sz = ftell(file);
        fseek(file, 0, SEEK_SET);
        char* data = new char[sz + 1];
        data[sz] = '\0';
        fread(data, sz, 1, file);
        fclose(file);
        return data;
    }

    return nullptr;
}

bool LoadShader(unsigned int& program, shaderinfo_t info)
{
    int ivStatus = 0;

    std::unique_ptr<char[]> _szVertexShader(ReadShaderFile(info.szVertexFileName));
    std::unique_ptr<char[]> _szFragShader(ReadShaderFile(info.szFragFileName));

    if (_szVertexShader == 0 || _szFragShader == 0)
    {
        return false;
    }

    const char* szVertexShader = _szVertexShader.get();
    const char* szFragShader = _szFragShader.get();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &szVertexShader, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &ivStatus);
    if (ivStatus == 0)
    {
        int infolen;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &infolen);

        char* infoBuffer = new char[infolen + 1];

        glGetShaderInfoLog(vertexShader, infolen + 1, NULL, infoBuffer);
        printf("[FSHADER] %s\n", infoBuffer);
        delete[] infoBuffer;
        return false;
    }

    unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &szFragShader, NULL);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &ivStatus);
    if (ivStatus == 0)
    {
        int infolen;
        glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &infolen);

        char* infoBuffer = new char[infolen + 1];

        glGetShaderInfoLog(fragShader, infolen + 1, NULL, infoBuffer);
        printf("[VSHADER] %s\n", infoBuffer);
        delete[] infoBuffer;
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        int infolen;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infolen);
        char* infoBuffer = new char[infolen + 1];
        glGetProgramInfoLog(program, 512, NULL, infoBuffer);
        printf("[PSHADER]: %s\n", infoBuffer);
        delete[] infoBuffer;
        return false;
    }

    glDeleteShader(fragShader);
    glDeleteShader(vertexShader);

    return true;
}