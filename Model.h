//
// Created by 郭珂桢 on 2024/5/29.
//

#ifndef MINECRAFT_MODEL_H
#define MINECRAFT_MODEL_H

#include "Mesh.h"
#include "Util/NonCopyable.h"
#include "Renderer/RenderInfo.h"

class Model : public NonCopyable
{
public:
    Model() = default;
    Model(const Mesh &mesh);
    ~Model();

    Model(Model &&other);
    Model &operator=(Model &&other);

    void AddData(const Mesh &mesh);

    void DeleteData();

    void GenVAO();
    void AddEBO(const std::vector<GLuint> &indices);
    void AddVBO(int dimensions, const std::vector<GLfloat> &data);
    void BindVAO() const;

    int GetIndicesCount() const;

    const RenderInfo &GetRenderInfo() const;

private:
    RenderInfo m_renderInfo;

    int m_vboCount = 0;
    std::vector<GLuint> m_buffers;
};


#endif //MINECRAFT_MODEL_H
