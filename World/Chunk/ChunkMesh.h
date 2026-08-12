//
// Created by 郭珂桢 on 25-8-1.
//

#ifndef MINECRAFT_CHUNKMESH_H
#define MINECRAFT_CHUNKMESH_H

#include "../../Model.h"

#include <array>
#include <vector>
#include <SFML/Graphics.hpp>

class ChunkMesh {
public:
    ChunkMesh() = default;

    void addFace(const std::array<GLfloat, 12> &blockFace,
                 const std::array<GLfloat, 8> &textureCoords,
                 const sf::Vector3i &chunkPosition,
                 const sf::Vector3i &blockPosition, GLfloat cardinalLight);

    void addFace(const std::array<GLfloat, 12> &blockFace,
                 const std::array<GLfloat, 8> &textureCoords,
                 const sf::Vector3i &chunkPosition,
                 const sf::Vector3i &blockPosition,
                 const std::array<GLfloat, 4> &vertexLights);

    // Drop CPU vertex data without touching GPU
    void clearCPU();

    // Upload CPU mesh to GPU (call on the GL thread)
    void bufferMesh();

    const Model &getModel() const;

    void deleteData();

    int faces = 0;

private:
    Mesh m_mesh;
    Model m_model;
    std::vector<GLfloat> m_light;
    GLuint m_indexIndex = 0;
};

struct ChunkMeshCollection {
    ChunkMesh solidMesh;
    ChunkMesh waterMesh;
    ChunkMesh floraMesh;
};


#endif //MINECRAFT_CHUNKMESH_H
