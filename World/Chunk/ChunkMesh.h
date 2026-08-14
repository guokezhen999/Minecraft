//
// Created by 郭珂桢 on 25-8-1.
//

#ifndef MINECRAFT_CHUNKMESH_H
#define MINECRAFT_CHUNKMESH_H

#include "../../Model.h"

#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace FaceId {
constexpr float PosX = 0.0f;
constexpr float NegX = 1.0f;
constexpr float PosY = 2.0f;
constexpr float NegY = 3.0f;
constexpr float PosZ = 4.0f;
constexpr float NegZ = 5.0f;
constexpr float FloraA = 6.0f;
constexpr float FloraB = 7.0f;
}

class ChunkMesh {
public:
    ChunkMesh() = default;

    void addFace(const std::array<GLfloat, 12> &blockFace,
                 const std::array<GLfloat, 8> &textureCoords,
                 const glm::ivec3 &chunkPosition,
                 const glm::ivec3 &blockPosition,
                 GLfloat shade, GLfloat sky, GLfloat block, GLfloat faceId);

    void addFace(const std::array<GLfloat, 12> &blockFace,
                 const std::array<GLfloat, 8> &textureCoords,
                 const glm::ivec3 &chunkPosition,
                 const glm::ivec3 &blockPosition,
                 const std::array<GLfloat, 4> &shade,
                 const std::array<GLfloat, 4> &sky,
                 const std::array<GLfloat, 4> &block,
                 GLfloat faceId);

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
    std::vector<GLfloat> m_normal;
    GLuint m_indexIndex = 0;
};

struct ChunkMeshCollection {
    ChunkMesh solidMesh;
    ChunkMesh waterMesh;
    ChunkMesh floraMesh;
};


#endif //MINECRAFT_CHUNKMESH_H
