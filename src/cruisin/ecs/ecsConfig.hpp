/*
 * Copyright (c) 2016 Galen Cochrane
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef EZECS_ECSCONFIG_HPP
#define EZECS_ECSCONFIG_HPP

// This doesn't really matter - it's just so IDE parsers can see the Component base class and stuff (for editing only).
#include "../../../extern/ezecs/source/ecsComponents.hpp"

// BEGIN INCLUDES

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "CNeuralNet.h"

// END INCLUDES

using namespace ezecs;

namespace {

  // BEGIN DECLARATIONS

  typedef Delegate<glm::mat4(const glm::mat4&, uint32_t time)> transformFunc;
  typedef std::vector<float>* floatVecPtr;

  struct Placement : public Component<Placement> {
    glm::mat4 mat {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    Placement(glm::mat4 mat);
    glm::vec3 getTranslation();
    glm::vec3 getLookAt();
  };

  struct TransformFunction : public Component<TransformFunction> {
    glm::mat4 transformed;
    transformFunc func;
    TransformFunction(transformFunc func);
  };
  EZECS_COMPONENT_DEPENDENCIES(TransformFunction, Placement)

  struct Perspective : public Component<Perspective> {
    float fovy, prevFovy,
        near, far;
    Perspective(float fovy, float near, float far);
  };
  EZECS_COMPONENT_DEPENDENCIES(Perspective, Placement)

  struct WheelInfo {
    entityId parentVehicle, myId;
    int bulletWheelId;
    float leftOrRight;
  };
  struct WheelInitInfo {
    WheelInfo wi;
    btVector3 connectionPoint, direction, axle;
    float suspensionRestLength, wheelRadius;
    bool isFrontWheel;
  };
  struct Physics : public Component<Physics> {
    enum Geometry {
      NONE, PLANE, SPHERE, BOX, MESH, TERRAIN, WHEEL
    };
    int geom;
    float mass;
    btCollisionShape* shape;
    btRigidBody* rigidBody;
    void* geomInitData;
    void* customData = NULL;
    Physics(float mass, void* geomData, Geometry geom);
    ~Physics();
    void setTransform(glm::mat4 &newTrans);
    void beStill();
  };
  EZECS_COMPONENT_DEPENDENCIES(Physics, Placement)

  struct PyramidControls : public Component<PyramidControls> {
    enum Style {
      ROTATE_ALL_AXES, ROTATE_ABOUT_Z
    };
    glm::vec3 accel;
    glm::vec3 force;
    glm::vec3 up;
    int style;
    PyramidControls(Style style);
  };
  EZECS_COMPONENT_DEPENDENCIES(PyramidControls, Physics)

  struct TrackControls : public Component<TrackControls> {
    glm::vec2 control;
    glm::vec2 torque;
    std::vector<WheelInfo> wheels;
    btRaycastVehicle *vehicle;
    btRaycastVehicle::btVehicleTuning tuning;
    TrackControls();
  };
  EZECS_COMPONENT_DEPENDENCIES(TrackControls, Physics)

  struct MouseControls : public Component<MouseControls> {
    float sinOfVertTolerance = 0.5;
    bool invertedX, invertedY;
    MouseControls(bool invertedX, bool invertedY);
  };
  EZECS_COMPONENT_DEPENDENCIES(MouseControls, Placement)

  struct Terrain : public Component<Terrain> {
    floatVecPtr heights;
    size_t resX, resY;
    float sclX, sclY, sclZ, minZ, maxZ;
    Terrain(floatVecPtr heights, size_t resX, size_t resY, float sclX, float sclY, float sclZ, float minZ, float maxZ);
  };
  EZECS_COMPONENT_DEPENDENCIES(Terrain, Placement)

  struct SweeperAi : public Component<SweeperAi> {
    CNeuralNet net;
    btCollisionShape* ghostShape;
    btGhostObject *ghostObject;
    float fitness = 0.f;
    float distance = 0.f;
    SweeperAi();
    void reset();
  };
  EZECS_COMPONENT_DEPENDENCIES(SweeperAi, TrackControls)

  struct SweeperTarget : public Component<SweeperTarget> {
    SweeperTarget();
  };
  EZECS_COMPONENT_DEPENDENCIES(SweeperTarget, Placement)

  // END DECLARATIONS

  // BEGIN DEFINITIONS

  Placement::Placement(glm::mat4 mat)
      : mat(mat) { }

  glm::vec3 Placement::getTranslation() {
    return glm::vec3(mat[3][0], mat[3][1], mat[3][2]);
  }

  glm::vec3 Placement::getLookAt() {
    glm::vec4 lookAt = mat * glm::vec4(0.f, 1.f, 0.f, 0.f);
    return glm::vec3(lookAt.x, lookAt.y, lookAt.z);
  }

  TransformFunction::TransformFunction(transformFunc func)
      : func(func) { }

  Perspective::Perspective(float fovy, float near, float far)
      : fovy(fovy), prevFovy(fovy), near(near), far(far) { }

  Physics::Physics(float mass, void* geomData, Physics::Geometry geom)
      : geom(geom), mass(mass), geomInitData(geomData) { }
  Physics::~Physics() {
    if (customData && geom == WHEEL) {
      switch (geom) {
        case WHEEL: {
          delete ((WheelInfo*)customData);
        } break;
        default: break; // TODO: Make this deconstructor safer, or do deallocation elsewhere
      }
    }
  }
  void Physics::setTransform(glm::mat4 &newTrans) {
    btTransform transform = rigidBody->getCenterOfMassTransform();
    transform.setFromOpenGLMatrix((btScalar *)&newTrans);
    rigidBody->setCenterOfMassTransform(transform);
  }
  void Physics::beStill() {
    rigidBody->setLinearVelocity( {0.f, 0.f, 0.f} );
    rigidBody->setAngularVelocity( {0.f, 0.f, 0.f} );
  }

  PyramidControls::PyramidControls(PyramidControls::Style style)
      : style(style) { }

  TrackControls::TrackControls() {}

  MouseControls::MouseControls(bool invertedX, bool invertedY)
      : invertedX(invertedX), invertedY(invertedY) { }

  Terrain::Terrain(floatVecPtr heights, size_t resX, size_t resY, float sclX, float sclY, float sclZ, float minZ, float maxZ)
      : heights(heights), resX(resX), resY(resY), sclX(sclX), sclY(sclY), sclZ(sclZ), minZ(minZ), maxZ(maxZ) { }

  SweeperAi::SweeperAi() { }
  void SweeperAi::reset() {
    fitness = 0.f;
  }

  SweeperTarget::SweeperTarget() { }

  // END DEFINITIONS

}

#endif //EZECS_ECSCONFIG_HPP
