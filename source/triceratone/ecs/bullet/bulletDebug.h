//
// Created by volundr on 4/2/17.
//

#pragma once

#include <epoxy/gl.h>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>

#include "configuration.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/type_ptr.hpp>

#include "sceneObject.h"
#include "openglValidation.h"
#include "shaderProgram.h"
#include "shaders.h"

namespace at3 {
  /**
   * Conversion from Bullet vector structures to glm vector structures
   */
  glm::vec3 bulletToGlm(const btVector3& vec);
  btVector3 glmToBullet(const glm::vec3& vec);

  /**
   * Allows bullet to draw debug stuff with our graphics backend.
   */
  template <typename EcsInterface>
  class BulletDebug : public btIDebugDraw, public SceneObject<EcsInterface> {
      typedef struct { float pos[3], color[3]; } LineVertex;
      typedef struct { LineVertex vertices[2]; } Line;
      typedef struct { float pos[3], color[3]; } Point;
      std::vector<Line> m_lines;
      std::vector<Point> m_points;
      GLuint m_lineBuffer, m_pointBuffer;
      bool m_linesChanged = false, m_pointsChanged = false;
      int m_debugMode;

      void m_updateLines();
      void m_updatePoints();
      void m_drawLines(
          const glm::mat4 &modelView,
          const glm::mat4 &projection) const;
      void m_drawPoints(
          const glm::mat4 &modelView,
          const glm::mat4 &projection) const;

      void m_queueLine(
          const glm::vec3 &a,
          const glm::vec3 &b,
          const glm::vec3 &color);
      void m_queuePoint(
          const glm::vec3 &pos,
          const glm::vec3 &color);
    public:
      BulletDebug();
      virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
      void drawLineGlm(const glm::vec3& from,const glm::vec3& to,const glm::vec3& color);
      virtual void drawContactPoint(const btVector3& pointOnB,const btVector3& normalOnB,btScalar distance,
                                    int lifeTime,const btVector3& color);
      virtual void reportErrorWarning(const char* warningString);
      virtual void draw3dText(const btVector3& location,const char* textString);
      virtual void setDebugMode(int debugMode);
      virtual inline int getDebugMode() const;
      void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                bool debug);
  };

  template <typename EcsInterface>
  BulletDebug<EcsInterface>::BulletDebug() : m_debugMode(0) {
    glGenBuffers(1, &m_lineBuffer);
    FORCE_ASSERT_GL_ERROR();
    glGenBuffers(1, &m_pointBuffer);
    FORCE_ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    m_queueLine(bulletToGlm(from), bulletToGlm(to), bulletToGlm(color));
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::drawLineGlm(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color) {
    m_queueLine(from, to, color);
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::drawContactPoint(const btVector3 &pointOnB, const btVector3 &normalOnB, btScalar distance,
                                                          int lifeTime, const btVector3 &color) {
    m_queuePoint(bulletToGlm(pointOnB), bulletToGlm(normalOnB));
  }
  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::reportErrorWarning(const char *warningString) {
    printf("%s\n", warningString);
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::draw3dText(const btVector3 &location, const char *textString) {

  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::setDebugMode(int debugMode) {
    m_debugMode = debugMode;
  }

  template <typename EcsInterface>
  inline int BulletDebug<EcsInterface>::getDebugMode() const {
    return m_debugMode;
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_updateLines() {
    // Upload the lines to the GL
    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,  // target
        m_lines.size() * sizeof(Line),  // size
        m_lines.data(),  // data
        GL_STATIC_DRAW  // usage
    );
    ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_updatePoints() {
    // Upload the lines to the GL
    glBindBuffer(GL_ARRAY_BUFFER, m_pointBuffer);
    ASSERT_GL_ERROR();
    glBufferData(
        GL_ARRAY_BUFFER,  // target
        m_points.size() * sizeof(Point),  // size
        m_points.data(),  // data
        GL_STATIC_DRAW  // usage
    );
    ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_drawLines(
      const glm::mat4 &modelView,
      const glm::mat4 &projection) const {
    // Use the wireframe shader
    auto shader = Shaders::wireframeShader();
    shader->use();

    // Prepare the uniform values
    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(modelView)  // value
    );
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(projection)  // value
    );
    ASSERT_GL_ERROR();

    // Prepare the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(LineVertex),  // stride
        &(((LineVertex *) 0)->pos[0])  // pointer
    );
    ASSERT_GL_ERROR();
    assert(shader->vertColorLocation() != -1);
    glEnableVertexAttribArray(shader->vertColorLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertColorLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(LineVertex),  // stride
        &(((LineVertex *) 0)->color[0])  // pointer
    );
    ASSERT_GL_ERROR();
    // Draw all of the lines in our line buffer
    glLineWidth(1.0f);
    ASSERT_GL_ERROR();
    glDrawArrays(
        GL_LINES,  // mode
        0,  // first
        m_lines.size() * 2  // count
    );
    ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_drawPoints(
      const glm::mat4 &modelView,
      const glm::mat4 &projection) const {
    // Use the billboard point shader
    auto shader = Shaders::billboardPointShader();
    shader->use();

    // Prepare the uniform values
    assert(shader->modelViewLocation() != -1);
    glUniformMatrix4fv(
        shader->modelViewLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(modelView)  // value
    );
    ASSERT_GL_ERROR();
    assert(shader->projectionLocation() != -1);
    glUniformMatrix4fv(
        shader->projectionLocation(),  // location
        1,  // count
        0,  // transpose
        glm::value_ptr(projection)  // value
    );
    ASSERT_GL_ERROR();

    // Prepare the vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, m_lineBuffer);
    ASSERT_GL_ERROR();
    assert(shader->vertPositionLocation() != -1);
    glEnableVertexAttribArray(shader->vertPositionLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertPositionLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(LineVertex),  // stride
        &(((LineVertex *) 0)->pos[0])  // pointer
    );
    ASSERT_GL_ERROR();
    assert(shader->vertColorLocation() != -1);
    glEnableVertexAttribArray(shader->vertColorLocation());
    ASSERT_GL_ERROR();
    glVertexAttribPointer(
        shader->vertColorLocation(),  // index
        3,  // size
        GL_FLOAT,  // type
        0,  // normalized
        sizeof(LineVertex),  // stride
        &(((LineVertex *) 0)->color[0])  // pointer
    );
    ASSERT_GL_ERROR();
    // Draw all of the lines in our line buffer
    glLineWidth(1.0f);
    ASSERT_GL_ERROR();
    glDrawArrays(
        GL_LINES,  // mode
        0,  // first
        m_lines.size() * 2  // count
    );
    ASSERT_GL_ERROR();
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_queueLine(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &color) {
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

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::m_queuePoint(const glm::vec3 &pos, const glm::vec3 &color) {
    Point point;
    point.pos[0] = pos.x;
    point.pos[1] = pos.y;
    point.pos[2] = pos.z;
    point.color[0] = color.r;
    point.color[1] = color.g;
    point.color[2] = color.b;
    m_points.push_back(point);
    m_pointsChanged = true;
  }

  template <typename EcsInterface>
  void BulletDebug<EcsInterface>::draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                                              bool debug)
  {
    if (m_linesChanged || m_pointsChanged) {
      // Calculate the model-view matrix
      glm::mat4 modelView = worldView * modelWorld;
      if (m_linesChanged) {
        m_updateLines();
        // Draw the lines
        m_drawLines(modelView, projection);
        m_lines.clear();
        m_linesChanged = false;
      }
      if (m_pointsChanged) {
        m_updatePoints();
        // Draw the lines
        m_drawPoints(modelView, projection);
        m_points.clear();
        m_pointsChanged = false;
      }
    }
  }
}
