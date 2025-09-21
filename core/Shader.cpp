#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

void Shader::Load(const std::string& vertPath, const std::string& fragPath) {
    std::string vertCode = LoadFromFile(vertPath);
    std::string fragCode = LoadFromFile(fragPath);

    unsigned int vertShader = CompileShader(GL_VERTEX_SHADER, vertCode);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragShader = CompileShader(GL_FRAGMENT_SHADER, fragCode);
    // check for shader compile errors
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertShader);
    glAttachShader(ID, fragShader);
    glLinkProgram(ID);
    // check for linking errors
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        std::cout << vertPath << std::endl;
        std::cout << fragPath << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

void Shader::Use() const {
    glUseProgram(ID);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

std::string Shader::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Shader::SetInt(const std::string& name, int value) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform1i(loc, value);
}

void Shader::SetFloat(const std::string& name, float value) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform1f(loc, value);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
    int loc = glGetUniformLocation(ID, name.c_str());
    glUniform3fv(loc, 1, &value[0]);
}

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

