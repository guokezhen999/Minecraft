//
// Created by 郭珂桢 on 2024/5/29.
//

#ifndef MINECRAFT_MESH_H
#define MINECRAFT_MESH_H

#include <glad/glad.h>
#include <vector>

struct Mesh
{
    std::vector<GLfloat> vertexPositions;
    std::vector<GLfloat> textureCoords;
    std::vector<GLuint> indices;
};

#endif //MINECRAFT_MESH_H
