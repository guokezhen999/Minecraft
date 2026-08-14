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

void ChunkRenderer::Render(const Camera& camera, bool underwater,
                           const Atmosphere& atmosphere) {
    if (m_solid.empty() && m_water.empty() && m_transparent.empty()) {
        return;
    }

    m_shader.Use();
    m_shader.SetProjectionView(camera.GetProjectionViewMatrix());
    m_shader.SetVector3f("cameraPos", camera.Position);
    m_shader.SetVector3f("fogColor", atmosphere.fogColor);
    m_shader.SetFloat("fogStart", atmosphere.fogStart);
    m_shader.SetFloat("fogEnd", atmosphere.fogEnd);
    m_shader.SetFloat("dayFactor", atmosphere.dayFactor);
    m_shader.SetVector3f("sunDir", atmosphere.sunDir);
    m_shader.SetVector3f("sunColor", atmosphere.sunColor);
    m_shader.SetVector3f("moonDir", atmosphere.moonDir);
    m_shader.SetVector3f("moonColor", atmosphere.moonColor);
    m_shader.SetVector3f("skyLightColor", atmosphere.skyLightColor);
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

    auto drawModels = [&](const std::vector<const Model*>& list) {
        for (const auto* model : list) {
            model->BindVAO();
            glDrawElements(GL_TRIANGLES, model->GetIndicesCount(),
                           GL_UNSIGNED_INT, 0);
        }
    };

    // ── Opaque ──────────────────────────────────────────────────────────────
    m_shader.SetInteger("liquidPass", 0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);

    drawModels(m_solid);

    glDisable(GL_CULL_FACE);

    // ── Water (need both sides underwater; no depth write) ──────────────────
    if (!m_water.empty()) {
        sortBackToFront(m_water);
        m_shader.SetInteger("liquidPass", 1);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (const auto& item : m_water) {
            item.model->BindVAO();
            glDrawElements(GL_TRIANGLES, item.model->GetIndicesCount(),
                           GL_UNSIGNED_INT, 0);
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // ── Flora (alpha discard, write depth, both sides) ──────────────────────
    if (!m_transparent.empty()) {
        m_shader.SetInteger("liquidPass", 0);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        for (const auto& item : m_transparent) {
            item.model->BindVAO();
            glDrawElements(GL_TRIANGLES, item.model->GetIndicesCount(),
                           GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    m_solid.clear();
    m_water.clear();
    m_transparent.clear();
}
