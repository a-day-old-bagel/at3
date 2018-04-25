
#include "textureView.h"
#include "settings.h"

namespace at3 {
  TextureView::TextureView() {
    // generate a quad that covers part of the screen
    float y = 1.f;
    float x = y * (settings::graphics::windowDimX / settings::graphics::windowDimY);
    float corners[18] = {
        -1.f, -1.f, 0.f,
           x, -1.f, 0.f,
        -1.f,    y, 0.f,
        -1.f,    y, 0.f,
           x, -1.f, 0.f,
           x,    y, 0.f
    };
    float texCorners[12] = {
        0.f, 1.f,
        1.f, 1.f,
        0.f, 0.f,
        0.f, 0.f,
        1.f, 1.f,
        1.f, 0.f
    };
    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, corners, GL_STATIC_DRAW);
    glGenBuffers(1, &m_texCoords);
    glBindBuffer(GL_ARRAY_BUFFER, m_texCoords);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, texCorners, GL_STATIC_DRAW);
  }

  void TextureView::viewTextureUnit(GLuint unit) {
    m_textureUnit = unit;
  }

  void TextureView::draw() {
    auto shader = Shaders::textureViewShader();
    shader->use();

    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(shader->vertPositionLocation(), 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    ASSERT_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, m_texCoords);
    ASSERT_GL_ERROR();
    assert(shader->vertTexCoordLocation() != -1);
    glEnableVertexAttribArray(shader->vertTexCoordLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(shader->vertTexCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    ASSERT_GL_ERROR();

    assert(shader->texture0() != -1);
    glUniform1i(shader->texture0(), m_textureUnit);
    ASSERT_GL_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_ERROR();
  }
}
