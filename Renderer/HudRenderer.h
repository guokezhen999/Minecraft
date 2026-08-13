//
// Shared ortho HUD: colored quads, optional texture, ASCII bitmap text
// Origin: top-left, Y increases downward (matches GLFW cursor coords)
//

#ifndef MINECRAFT_HUDRENDERER_H
#define MINECRAFT_HUDRENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>

class Shader;

class HudRenderer {
public:
    static constexpr int FONT_CELL = 8;
    static constexpr int FONT_COLS = 16;

    HudRenderer();
    ~HudRenderer();

    HudRenderer(const HudRenderer&) = delete;
    HudRenderer& operator=(const HudRenderer&) = delete;

    void begin(int windowWidth, int windowHeight);
    void end();

    void drawQuad(float x0, float y0, float x1, float y1, const glm::vec4& color);
    void drawTexturedQuad(float x0, float y0, float x1, float y1,
                          float u0, float v0, float u1, float v1,
                          const glm::vec4& color);
    void drawText(float x, float y, const std::string& str, float scale,
                  const glm::vec4& color);

    float textWidth(const std::string& str, float scale) const;
    float textHeight(float scale) const { return FONT_CELL * scale; }

    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    void init();
    void emitQuad(float x0, float y0, float x1, float y1,
                  float u0, float v0, float u1, float v1);
    bool loadFontTexture();

    Shader* m_shader = nullptr;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_fontTex = 0;
    int m_width = 0;
    int m_height = 0;
    bool m_active = false;
};

#endif // MINECRAFT_HUDRENDERER_H
