//
// Created by volundr on 4/2/17.
//

#ifndef AT3_BULLETDEBUG_H
#define AT3_BULLETDEBUG_H

#include <epoxy/gl.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include "sceneObject.h"

namespace at3 {
  /*
   * Conversion from Bullet vector structures to glm vector structures
   */
  glm::vec3 bulletToGlm(const btVector3& vec);

  /*
   * Allows bullet to draw debug stuff with our graphics backend.
   */
  class BulletDebug : public btIDebugDraw, public SceneObject {
      typedef struct { float pos[3], color[3]; } LineVertex;
      typedef struct { LineVertex vertices[2]; } Line;
      typedef struct { float pos[3], color[3]; } Point;
      std::vector<Line> m_lines;
      std::vector<Point> m_points;
      GLuint m_lineBuffer, m_pointBuffer;
      bool m_linesChanged = false, m_pointsChanged = false;
      int m_debugMode;
      ezecs::State* state;

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
      BulletDebug(ezecs::State* state);
      virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
      virtual void drawContactPoint(const btVector3& pointOnB,const btVector3& normalOnB,btScalar distance,
                                    int lifeTime,const btVector3& color);
      virtual void reportErrorWarning(const char* warningString);
      virtual void draw3dText(const btVector3& location,const char* textString);
      virtual void setDebugMode(int debugMode);
      virtual inline int getDebugMode() const;
      void draw(const glm::mat4 &modelWorld, const glm::mat4 &worldView, const glm::mat4 &projection,
                bool debug);
  };
}

#endif //AT3_BULLETDEBUG_H
