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
#include "./extern/ezecs/source/ecsComponents.hpp"

// BEGIN INCLUDES

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
class btCollisionShape;
class btDefaultMotionState;
class btRigidBody;

// END INCLUDES

using namespace ezecs;

namespace {

  // BEGIN DECLARATIONS

  typedef Delegate<glm::vec3(const glm::vec3 &, uint32_t)> scalarMultFuncDlgt;

  struct Position : public Component<Position> {
    glm::vec3 vec, lastVec;
    Position(glm::vec3 vec);
    glm::vec3 getVec(float alpha);
  };

  struct Scale : public Component<Scale> {
    glm::vec3 vec, lastVec;
    Scale(glm::vec3 vec);
  };

  struct ScalarMultFunc : public Component<ScalarMultFunc> {
    Delegate<glm::vec3(const glm::vec3&, uint32_t time)> multByFuncOfTime;
    ScalarMultFunc(scalarMultFuncDlgt func);
  };
  EZECS_COMPONENT_DEPENDENCIES(ScalarMultFunc, Scale)

  struct Orientation : public Component<Orientation> {
    glm::quat quat, lastQuat;
    Orientation(glm::quat quat);
    glm::quat getQuat(float alpha);
  };

  struct Perspective : public Component<Perspective> {
    float fovy, prevFovy,
        near, far;
    Perspective(float fovy, float near, float far);
  };
  EZECS_COMPONENT_DEPENDENCIES(Perspective, Position, Orientation)

  struct WasdControls : public Component<WasdControls> {
    enum Style {
      ROTATE_ALL_AXES, ROTATE_ABOUT_Z
    };
    glm::vec3 accel;
    entityId orientationProxy;
    int style;
    WasdControls(entityId orientationProxy, Style style);
  };
  EZECS_COMPONENT_DEPENDENCIES(WasdControls, Orientation)

  struct MouseControls : public Component<MouseControls> {
    bool invertedX, invertedY;
    MouseControls(bool invertedX, bool invertedY);
  };
  EZECS_COMPONENT_DEPENDENCIES(MouseControls, Orientation)

  struct Physics : public Component<Physics> {
    enum Geometry {
      NONE, PLANE, SPHERE, MESH
    };
    int geom;
    float mass;
    btCollisionShape* shape;
    btRigidBody* rigidBody;
    void* geomInitData;
    Physics(float mass, void* geomData, Geometry geom);
  };
  EZECS_COMPONENT_DEPENDENCIES(Physics, Position, Orientation)

  // END DECLARATIONS

  // BEGIN DEFINITIONS

  Position::Position(glm::vec3 vec)
      : vec(vec), lastVec(vec) { }
  glm::vec3 Position::getVec(float alpha) {
    return glm::mix(lastVec, vec, alpha);
  }

  Scale::Scale(glm::vec3 vec)
      : vec(vec), lastVec(vec) { }

  ScalarMultFunc::ScalarMultFunc(scalarMultFuncDlgt func)
      : multByFuncOfTime(func) { }

  Orientation::Orientation(glm::quat quat)
      : quat(quat), lastQuat(quat) { }
  glm::quat Orientation::getQuat(float alpha) {
    return glm::slerp(lastQuat, quat, alpha);
  }

  Perspective::Perspective(float fovy, float near, float far)
      : fovy(fovy), prevFovy(fovy), near(near), far(far) { }

  WasdControls::WasdControls(entityId orientationProxy, WasdControls::Style style)
      : orientationProxy(orientationProxy), style(style) { }

  MouseControls::MouseControls(bool invertedX, bool invertedY)
      : invertedX(invertedX), invertedY(invertedY) { }

  Physics::Physics(float mass, void* geomData, Physics::Geometry geom)
      : geom(geom), mass(mass), geomInitData(geomData) { }

  // END DEFINITIONS

}

#endif //EZECS_ECSCONFIG_HPP
