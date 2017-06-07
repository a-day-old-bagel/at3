#pragma once

#include <epoxy/gl.h>
#include <memory>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

#include "sceneObject.h"
#include "debug.h"
#include "openglValidation.h"
#include "shaderProgram.h"
#include "shaders.h"

namespace at3 {
  template<typename EcsInterface>
  class Debug : public SceneObject<EcsInterface> {
    private:
      typedef struct {
        float pos[3], color[3];
      } Point;
      typedef struct {
        Point vertices[2];
      } Line;
      std::vector<Line> m_lines;
      std::vector<Point> m_points;
      bool m_linesChanged;

      GLuint m_lineBuffer, m_pointBuffer;

      Debug();

      void m_updateLines();

      void m_updatePoints();

      void m_drawLines(
          const glm::mat4 &modelView,
          const glm::mat4 &projection) const;

      void m_drawPoints(
          const glm::mat4 &modelView,
          const glm::mat4 &projection) const;

      void m_drawLine(
          const glm::vec3 &a,
          const glm::vec3 &b,
          const glm::vec3 &color);

      void m_drawPoint(
          const glm::vec3 &pos,
          const glm::vec3 &color);

    public:
      static std::shared_ptr<Debug<EcsInterface>> instance();

      static void drawLine(
          const glm::vec3 &a,
          const glm::vec3 &b,
          const glm::vec3 &color) {
        instance()->m_drawLine(a, b, color);
      }

      static void drawPoint(
          const glm::vec3 &pos,
          const glm::vec3 &color) {
        instance()->m_drawPoint(pos, color);
      }

      void draw(const glm::mat4 &modelWorld,
                const glm::mat4 &worldView, const glm::mat4 &projection, bool debug);
  };

  template<typename EcsInterface>
  Debug<EcsInterface>::Debug() : m_linesChanged(true) {
    glGenBuffers(1, &m_lineBuffer);
    FORCE_ASSERT_GL_ERROR();
    glGenBuffers(1, &m_pointBuffer);
    FORCE_ASSERT_GL_ERROR();
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::m_updateLines() {
    // Upload the lines to the GL
    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,               // target
        m_lines.size() * sizeof(Line), // size
        m_lines.data(),                // data
        GL_STATIC_DRAW                 // usage
    );
    ASSERT_GL_ERROR();
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::m_updatePoints() {
    glBindBuffer(GL_ARRAY_BUFFER, m_pointBuffer);
    ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,                 // target
        m_points.size() * sizeof(Point), // size
        m_points.data(),                 // data
        GL_STATIC_DRAW                   // usage
    );
    ASSERT_GL_ERROR();
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::m_drawLines(
      const glm::mat4 &modelView,
      const glm::mat4 &projection) const {

    auto shader = Shaders::wireframeShader();
    shader->use();

    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(), // location
        1,                           // count
        0,                           // transpose
        glm::value_ptr(modelView)    // value
    );
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(), // location
        1,                            // count
        0,                            // transpose
        glm::value_ptr(projection)    // value
    );
    ASSERT_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(), // index
        3,                              // size
        GL_FLOAT,                       // type
        0,                              // normalized
        sizeof(Point),             // stride
        &(((Point *) 0)->pos[0])   // pointer
    );
    ASSERT_GL_ERROR();
    assert(shader->vertColorLocation() != -1);
    glEnableVertexAttribArray(shader->vertColorLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertColorLocation(),    // index
        3,                              // size
        GL_FLOAT,                       // type
        0,                              // normalized
        sizeof(Point),             // stride
        &(((Point *) 0)->color[0]) // pointer
    );
    ASSERT_GL_ERROR();

    glLineWidth(1.0f);
    ASSERT_GL_ERROR();
    glDrawArrays(
        GL_LINES,          // mode
        0,                 // first
        m_lines.size() * 2 // count
    );
    ASSERT_GL_ERROR();
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::m_drawPoints(
      const glm::mat4 &modelView,
      const glm::mat4 &projection) const {
    auto shader = Shaders::billboardPointShader();

    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(), // location
        1,                           // count
        0,                           // transpose
        glm::value_ptr(modelView)    // value
    );
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(), // location
        1,                            // count
        0,                            // transpose
        glm::value_ptr(projection)    // value
    );
    ASSERT_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(), // index
        3,                              // size
        GL_FLOAT,                       // type
        0,                              // normalized
        sizeof(Point),                  // stride
        &(((Point *) 0)->pos[0])        // pointer
    );
    ASSERT_GL_ERROR();
    assert(shader->vertColorLocation() != -1);
    glEnableVertexAttribArray(shader->vertColorLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertColorLocation(),    // index
        3,                              // size
        GL_FLOAT,                       // type
        0,                              // normalized
        sizeof(Point),                  // stride
        &(((Point *) 0)->color[0])      // pointer
    );
    ASSERT_GL_ERROR();

    glLineWidth(1.0f);
    ASSERT_GL_ERROR();
    glDrawArrays(
        GL_LINES,           // mode
        0,                  // first
        m_lines.size() * 2  // count
    );
    ASSERT_GL_ERROR();
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::m_drawLine(
      const glm::vec3 &a,
      const glm::vec3 &b,
      const glm::vec3 &color) {
    Line line;
    line.vertices[0].pos[0] = a.x;
    line.vertices[0].pos[1] = a.y;
    line.vertices[0].pos[2] = a.z;
    line.vertices[0].color[0] = color.x;
    line.vertices[0].color[1] = color.y;
    line.vertices[0].color[2] = color.z;
    line.vertices[1].pos[0] = b.x;
    line.vertices[1].pos[1] = b.y;
    line.vertices[1].pos[2] = b.z;
    line.vertices[1].color[0] = color.x;
    line.vertices[1].color[1] = color.y;
    line.vertices[1].color[2] = color.z;
    m_lines.push_back(line);
    m_linesChanged = true;
  }

  template<typename EcsInterface>
  std::shared_ptr<Debug<EcsInterface>> Debug<EcsInterface>::instance() {
    static std::shared_ptr<Debug> instance = std::shared_ptr<Debug>(
        new Debug());
    return instance;
  }

  template<typename EcsInterface>
  void Debug<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                                 bool debug) {
    if (m_linesChanged) {
      m_updateLines();
      m_linesChanged = false;
    }

    glm::mat4 modelView = worldView * modelWorld;

    m_drawLines(modelView, projection);
  }
}
