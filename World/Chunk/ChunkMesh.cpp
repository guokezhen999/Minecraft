//
// Created by 郭珂桢 on 25-8-1.
//

#include "ChunkMesh.h"
#include "../WorldConstants.h"

namespace {

glm::vec3 normalFromFaceId(GLfloat faceId) {
    switch (static_cast<int>(faceId + 0.5f)) {
    case 0: return { 1.0f,  0.0f,  0.0f};
    case 1: return {-1.0f,  0.0f,  0.0f};
    case 2: return { 0.0f,  1.0f,  0.0f};
    case 3: return { 0.0f, -1.0f,  0.0f};
    case 4: return { 0.0f,  0.0f,  1.0f};
    case 5: return { 0.0f,  0.0f, -1.0f};
    case 6: return {-0.7071f, 0.0f, 0.7071f};
    default: return { 0.7071f, 0.0f, 0.7071f};
    }
}

} // namespace

void ChunkMesh::addFace(const std::array<GLfloat, 12> &blockFace,
                        const std::array<GLfloat, 8> &textureCoords,
                        const glm::ivec3 &chunkPosition,
                        const glm::ivec3 &blockPosition,
                        GLfloat shade, GLfloat sky, GLfloat block, GLfloat faceId) {
    addFace(blockFace, textureCoords, chunkPosition, blockPosition,
            std::array<GLfloat, 4>{shade, shade, shade, shade},
            std::array<GLfloat, 4>{sky, sky, sky, sky},
            std::array<GLfloat, 4>{block, block, block, block},
            faceId);
}

void ChunkMesh::addFace(const std::array<GLfloat, 12> &blockFace,
                        const std::array<GLfloat, 8> &textureCoords,
                        const glm::ivec3 &chunkPosition,
                        const glm::ivec3 &blockPosition,
                        const std::array<GLfloat, 4> &shade,
                        const std::array<GLfloat, 4> &sky,
                        const std::array<GLfloat, 4> &block,
                        GLfloat faceId) {
    faces++;
    auto &vertices = m_mesh.vertexPositions;
    auto &texCoords = m_mesh.textureCoords;
    auto &indices = m_mesh.indices;
    const glm::vec3 n = normalFromFaceId(faceId);

    texCoords.insert(texCoords.end(), textureCoords.begin(), textureCoords.end());

    for (int i = 0, index = 0; i < 4; ++i) {
        vertices.push_back(blockFace[index++] + chunkPosition.x * CHUNK_SIZE + blockPosition.x);
        vertices.push_back(blockFace[index++] + chunkPosition.y * CHUNK_SIZE +
                            blockPosition.y);
        vertices.push_back(blockFace[index++] + chunkPosition.z * CHUNK_SIZE +
                            blockPosition.z);
        m_light.push_back(shade[i]);
        m_light.push_back(sky[i]);
        m_light.push_back(block[i]);
        m_normal.push_back(n.x);
        m_normal.push_back(n.y);
        m_normal.push_back(n.z);
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
    m_normal.clear();
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
    m_model.AddVBO(3, m_light);
    m_model.AddVBO(3, m_normal);
    clearCPU();
}

void ChunkMesh::deleteData() {
    m_model.DeleteData();
}

const Model &ChunkMesh::getModel() const {
    return m_model;
}
