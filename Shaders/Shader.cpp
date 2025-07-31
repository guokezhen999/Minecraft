//
// Created by 郭珂桢 on 2024/5/20.
//

#include "Shader.h"

#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const std::string &vertexFile, const std::string &fragmentFile)
: ID(loadShaderFromFile(vertexFile, fragmentFile))
{
    Use();
}

Shader::~Shader()
{
    glDeleteProgram(this->ID);
}

void Shader::Use() const
{
    glUseProgram(this->ID);
}

unsigned int Shader::loadShaderFromFile(const std::string &vertexFile, const std::string &fragmentFile)
{
    std::string vertexCode;
    std::string fragmentCode;
    try
    {
        // 打开文件
        std::ifstream vertexShaderFile(vertexFile);
        std::ifstream fragmentShaderFile(fragmentFile);
        std::stringstream vShaderStream, fShaderStream;
        // 从文件读入流
        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();
        // 关闭文件
        vertexShaderFile.close();
        fragmentShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::exception e)
    {
        std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    compile(vShaderCode, fShaderCode);
    return this->ID;
}

void Shader::compile(const char *vertexSource, const char *fragmentSource)
{
    GLuint sVertex, sFragment;
    // Vertex Shader
    sVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sVertex, 1, &vertexSource, NULL);
    glCompileShader(sVertex);
    checkCompileErrors(sVertex, "VERTEX");
    // Fragment Shader
    sFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sFragment, 1, &fragmentSource, NULL);
    glCompileShader(sFragment);
    checkCompileErrors(sFragment, "FRAGMENT");
    // shader program
    this->ID = glCreateProgram();
    glAttachShader(this->ID, sVertex);
    glAttachShader(this->ID, sFragment);
    glLinkProgram(this->ID);
    checkCompileErrors(this->ID, "PROGRAM");
    // delete Shaders
    glDeleteShader(sVertex);
    glDeleteShader(sFragment);
}


void Shader::checkCompileErrors(GLuint object, std::string type)
{
    GLint success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, NULL, infoLog);
            std::cout << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, NULL, infoLog);
            std::cout << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    }
}


void Shader::SetFloat(unsigned int location, GLfloat value)
{
    glUniform1f(location, value);
}

void Shader::SetInteger(unsigned int location, GLint value)
{
    glUniform1i(location, value);
}

void Shader::SetVector2f(unsigned int location, GLfloat x, GLfloat y)
{
    glUniform2f(location, x, y);
}

void Shader::SetVector2f(unsigned int location, const glm::vec2 &value)
{
    glUniform2f(location, value.x, value.y);
}

void Shader::SetVector3f(unsigned int location, GLfloat x, GLfloat y, GLfloat z)
{
    glUniform3f(location, x, y, z);
}

void Shader::SetVector3f(unsigned int location, const glm::vec3 &value)
{
    glUniform3f(location, value.x, value.y, value.z);
}

void Shader::SetVector4f(unsigned int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    glUniform4f(location, x, y, z, w);
}

void Shader::SetVector4f(unsigned int location, const glm::vec4 &value)
{
    glUniform4f(location, value.x, value.y, value.z, value.w);
}

void Shader::SetMatrix4f(unsigned int location, const glm::mat4 &matrix)
{
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(matrix));
}

void Shader::SetFloat(const char *name, GLfloat value)
{
    glUniform1f(glGetUniformLocation(this->ID, name), value);
}

void Shader::SetInteger(const char *name, GLint value)
{

    glUniform1i(glGetUniformLocation(this->ID, name), value);
}

void Shader::SetVector2f(const char *name, GLfloat x, GLfloat y)
{

    glUniform2f(glGetUniformLocation(this->ID, name), x, y);
}

void Shader::SetVector2f(const char *name, const glm::vec2 &value)
{

    glUniform2f(glGetUniformLocation(this->ID, name), value.x, value.y);
}

void Shader::SetVector3f(const char *name, GLfloat x, GLfloat y, GLfloat z)
{

    glUniform3f(glGetUniformLocation(this->ID, name), x, y, z);
}

void Shader::SetVector3f(const char *name, const glm::vec3 &value)
{

    glUniform3f(glGetUniformLocation(this->ID, name), value.x, value.y, value.z);
}

void Shader::SetVector4f(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{

    glUniform4f(glGetUniformLocation(this->ID, name), x, y, z, w);

}

void Shader::SetVector4f(const char *name, const glm::vec4 &value)
{

    glUniform4f(glGetUniformLocation(this->ID, name), value.x, value.y, value.z, value.w);

}

void Shader::SetMatrix4f(const char *name, const glm::mat4 &matrix)
{
    glUniformMatrix4fv(glGetUniformLocation(this->ID, name), 1,
                       GL_FALSE, glm::value_ptr(matrix));
}


