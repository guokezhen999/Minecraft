//
// Created by 郭珂桢 on 2024/5/20.
//

#ifndef MINECRAFT_SHADER_H
#define MINECRAFT_SHADER_H

#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <string>

#include "../Util/NonCopyable.h"

class Shader
{
public:
    GLuint ID;
    Shader() {}
    Shader(const std::string &vertexFile, const std::string &fragmentFile);

    virtual ~Shader();

    void Use() const;
    virtual void GetUniforms() = 0;

    void SetFloat(unsigned int location, GLfloat value);
    void SetInteger(unsigned int location, GLint value);
    void SetVector2f(unsigned int location, GLfloat x, GLfloat y);
    void SetVector2f(unsigned int location, const glm::vec2 &value);
    void SetVector3f(unsigned int location, GLfloat x, GLfloat y, GLfloat z);
    void SetVector3f(unsigned int location, const glm::vec3 &value);
    void SetVector4f(unsigned int location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void SetVector4f(unsigned int location, const glm::vec4 &value);
    void SetMatrix4f(unsigned int location, const glm::mat4 &matrix);

    void SetFloat(const char *name,  GLfloat value);
    void SetInteger(const char *name, GLint value);
    void SetVector2f(const char *name, GLfloat x, GLfloat y);
    void SetVector2f(const char *name, const glm::vec2 &value);
    void SetVector3f(const char *name, GLfloat x, GLfloat y, GLfloat z);
    void SetVector3f(const char *name, const glm::vec3 &value);
    void SetVector4f(const char *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void SetVector4f(const char *name, const glm::vec4 &value);
    void SetMatrix4f(const char *name, const glm::mat4 &matrix);

private:
    void compile(const char *vertexSource, const char *fragmentSource);
    unsigned int loadShaderFromFile(const std::string& vertexFile, const std::string& fragmentFile);
    void checkCompileErrors(GLuint object, std::string type);
};


#endif //MINECRAFT_SHADER_H
