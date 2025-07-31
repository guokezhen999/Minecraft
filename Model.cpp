//
// Created by 郭珂桢 on 2024/5/29.
//

#include "Model.h"

Model::Model(const Mesh &mesh)
{
    AddData(mesh);
}

Model::~Model()
{
    DeleteData();
}

Model::Model(Model &&other)
: m_renderInfo(other.m_renderInfo),
m_vboCount(other.m_vboCount),
m_buffers(std::move(other.m_buffers))
{
    other.m_renderInfo.Reset();
    other.m_vboCount = 0;
}

Model &Model::operator=(Model &&other)
{
    m_renderInfo = other.m_renderInfo;
    m_vboCount = other.m_vboCount;
    m_buffers = std::move(other.m_buffers);

    other.m_renderInfo.Reset();
    other.m_vboCount = 0;
}

void Model::AddData(const Mesh &mesh)
{
    GenVAO();

    AddVBO(3, mesh.vertexPositions);
    AddVBO(2, mesh.textureCoords);
    AddEBO(mesh.indices);
}

void Model::DeleteData()
{
    if (m_renderInfo.VAO)
        glDeleteVertexArrays(1, &m_renderInfo.VAO);
    if (m_buffers.size() > 0)
        glDeleteFramebuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
    m_buffers.clear();

    m_vboCount = 0;
    m_renderInfo.Reset();
}

void Model::GenVAO()
{
    if (m_renderInfo.VAO != 0)
        DeleteData();
    glGenVertexArrays(1, &m_renderInfo.VAO);
    glBindVertexArray(m_renderInfo.VAO);
}

void Model::AddEBO(const std::vector<GLuint> &indices)
{
    m_renderInfo.IndicesCount = static_cast<GLuint>(indices.size());
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);
}

void Model::AddVBO(int dimensions, const std::vector<GLfloat> &data)
{
    GLuint VBO;
    glGenBuffers(GL_ARRAY_BUFFER, &VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(GLfloat), data.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(static_cast<GLuint>(m_vboCount), dimensions, GL_FLOAT,
                          GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(m_vboCount++));

    m_buffers.push_back(VBO);
}

void Model::BindVAO() const
{
    glBindVertexArray(m_renderInfo.VAO);
}

int Model::GetIndicesCount() const
{
    return m_renderInfo.IndicesCount;
}

const RenderInfo &Model::GetRenderInfo() const
{
    return m_renderInfo;
}
