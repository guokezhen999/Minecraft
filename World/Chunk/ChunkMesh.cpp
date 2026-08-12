//
// Created by 郭珂桢 on 25-8-1.
//

#include "ChunkMesh.h"
#include "../WorldConstants.h"

void ChunkMesh::addFace(const std::array<GLfloat, 12> &blockFace,
                        const std::array<GLfloat, 8> &textureCoords,
                        const sf::Vector3i &chunkPosition,
                        const sf::Vector3i &blockPosition,
                        GLfloat cardinalLight) {
    addFace(blockFace, textureCoords, chunkPosition, blockPosition,
            std::array<GLfloat, 4>{cardinalLight, cardinalLight,
                                   cardinalLight, cardinalLight});
}

void ChunkMesh::addFace(const std::array<GLfloat, 12> &blockFace,
                        const std::array<GLfloat, 8> &textureCoords,
                        const sf::Vector3i &chunkPosition,
                        const sf::Vector3i &blockPosition,
                        const std::array<GLfloat, 4> &vertexLights) {
    faces++;
    auto &vertices = m_mesh.vertexPositions;
    auto &texCoords = m_mesh.textureCoords;
    auto &indices = m_mesh.indices;

    texCoords.insert(texCoords.end(), textureCoords.begin(), textureCoords.end());

    for (int i = 0, index = 0; i < 4; ++i) {
        vertices.push_back(blockFace[index++] + chunkPosition.x * CHUNK_SIZE + blockPosition.x);
        vertices.push_back(blockFace[index++] + chunkPosition.y * CHUNK_SIZE +
                            blockPosition.y);
        vertices.push_back(blockFace[index++] + chunkPosition.z * CHUNK_SIZE +
                            blockPosition.z);
        m_light.push_back(vertexLights[i]);
    }

    indices.insert(indices.end(), {m_indexIndex, m_indexIndex + 1, m_indexIndex + 2,
                                   m_indexIndex + 2, m_indexIndex + 3, m_indexIndex});
    m_indexIndex += 4;
}

void ChunkMesh::clearCPU() {
    m_mesh.vertexPositions.clear();
    m_mesh.textureCoords.clear();
    m_mesh.indices.clear();
    m_light.clear();
    m_indexIndex = 0;
    faces = 0;
}

void ChunkMesh::bufferMesh() {
    if (faces == 0) {
        m_model.DeleteData();
        clearCPU();
        return;
    }

    m_model.AddData(m_mesh);
    m_model.AddVBO(1, m_light);
    clearCPU();
}

void ChunkMesh::deleteData() {
    m_model.DeleteData();
}

const Model &ChunkMesh::getModel() const {
    return m_model;
}