//
// Created by 郭珂桢 on 2024/5/29.
//

#include "ChunkRenderer.h"
#include "../World/Chunk/ChunkMesh.h"
#include "../Camera.h"
#include "../ResourceManager.h"

#include <algorithm>
#include <glad/glad.h>

void ChunkRenderer::AddSolid(const ChunkMesh& mesh) {
    if (mesh.getModel().GetIndicesCount() > 0) {
        m_solid.push_back(&mesh.getModel());
    }
}

void ChunkRenderer::AddTransparent(const ChunkMesh& mesh, float distSq) {
    if (mesh.getModel().GetIndicesCount() > 0) {
        m_transparent.push_back({&mesh.getModel(), distSq});
    }
}

void ChunkRenderer::Render(const Camera& camera) {
    if (m_solid.empty() && m_transparent.empty()) {
        return;
    }

    m_shader.Use();
    m_shader.SetProjectionView(camera.GetProjectionViewMatrix());

    glActiveTexture(GL_TEXTURE0);
    ResourceManager::BlockTexture->Bind();
    m_shader.SetInteger("texture1", 0);

    // ── Opaque ──────────────────────────────────────────────────────────────
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    for (const auto* model : m_solid) {
        model->BindVAO();
        glDrawElements(GL_TRIANGLES, model->GetIndicesCount(), GL_UNSIGNED_INT, 0);
    }

    // ── Transparent (water / flora), far → near ─────────────────────────────
    if (!m_transparent.empty()) {
        std::sort(m_transparent.begin(), m_transparent.end(),
                  [](const TransparentDraw& a, const TransparentDraw& b) {
                      return a.distSq > b.distSq;
                  });

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const auto& item : m_transparent) {
            item.model->BindVAO();
            glDrawElements(GL_TRIANGLES, item.model->GetIndicesCount(),
                           GL_UNSIGNED_INT, 0);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glBindVertexArray(0);
    m_solid.clear();
    m_transparent.clear();
}
