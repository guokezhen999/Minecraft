//
// Created by 郭珂桢 on 2024/5/29.
//

#include "ChunkRenderer.h"
#include "../World/Chunk/ChunkMesh.h"
#include "../World/WorldConstants.h"
#include "../Camera.h"
#include "../ResourceManager.h"

#include <algorithm>
#include <glad/glad.h>

void ChunkRenderer::AddSolid(const ChunkMesh& mesh) {
    if (mesh.getModel().GetIndicesCount() > 0) {
        m_solid.push_back(&mesh.getModel());
    }
}

void ChunkRenderer::AddWater(const ChunkMesh& mesh, float distSq) {
    if (mesh.getModel().GetIndicesCount() > 0) {
        m_water.push_back({&mesh.getModel(), distSq});
    }
}

void ChunkRenderer::AddTransparent(const ChunkMesh& mesh, float distSq) {
    if (mesh.getModel().GetIndicesCount() > 0) {
        m_transparent.push_back({&mesh.getModel(), distSq});
    }
}

void ChunkRenderer::Render(const Camera& camera, bool underwater) {
    if (m_solid.empty() && m_water.empty() && m_transparent.empty()) {
        return;
    }

    m_shader.Use();
    m_shader.SetProjectionView(camera.GetProjectionViewMatrix());
    m_shader.SetVector3f("cameraPos", camera.Position);
    m_shader.SetVector3f("fogColor", FOG_R, FOG_G, FOG_B);
    m_shader.SetFloat("fogStart", FOG_START);
    m_shader.SetFloat("fogEnd", FOG_END);
    m_shader.SetInteger("underwater", underwater ? 1 : 0);
    m_shader.SetVector3f("underwaterFogColor",
                         UNDERWATER_FOG_R, UNDERWATER_FOG_G, UNDERWATER_FOG_B);
    m_shader.SetFloat("underwaterFogStart", UNDERWATER_FOG_START);
    m_shader.SetFloat("underwaterFogEnd", UNDERWATER_FOG_END);
    m_shader.SetVector3f("underwaterTint",
                         UNDERWATER_TINT_R, UNDERWATER_TINT_G, UNDERWATER_TINT_B);

    glActiveTexture(GL_TEXTURE0);
    ResourceManager::BlockTexture->Bind();
    m_shader.SetInteger("texture1", 0);

    auto sortBackToFront = [](std::vector<TransparentDraw>& list) {
        std::sort(list.begin(), list.end(),
                  [](const TransparentDraw& a, const TransparentDraw& b) {
                      return a.distSq > b.distSq;
                  });
    };

    auto drawTransparent = [&](std::vector<TransparentDraw>& list, int liquidPass) {
        if (list.empty())
            return;
        sortBackToFront(list);
        m_shader.SetInteger("liquidPass", liquidPass);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (const auto& item : list) {
            item.model->BindVAO();
            glDrawElements(GL_TRIANGLES, item.model->GetIndicesCount(),
                           GL_UNSIGNED_INT, 0);
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    };

    // ── Opaque ──────────────────────────────────────────────────────────────
    m_shader.SetInteger("liquidPass", 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    for (const auto* model : m_solid) {
        model->BindVAO();
        glDrawElements(GL_TRIANGLES, model->GetIndicesCount(), GL_UNSIGNED_INT, 0);
    }

    // ── Water (surface vs submerged look) ───────────────────────────────────
    drawTransparent(m_water, 1);

    // ── Flora ───────────────────────────────────────────────────────────────
    drawTransparent(m_transparent, 0);

    glBindVertexArray(0);
    m_solid.clear();
    m_water.clear();
    m_transparent.clear();
}
