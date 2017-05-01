//
// Created by volundr on 4/27/17.
//

#include <glm/gtc/matrix_transform.hpp>
#include "duneBuggy.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

#define WHEEL_RADIUS 1.5f

using namespace ezecs;

namespace at3 {

  static glm::mat4 wheelScaler(const glm::mat4& transformIn, uint32_t time) {
    return glm::scale(glm::mat4(), {0.5f, WHEEL_RADIUS, WHEEL_RADIUS});
  }
  static glm::mat4 chassisScalar(const glm::mat4& transformIn, uint32_t time) {
    return glm::scale(glm::mat4(), {1.8f, 2.8f, 1.f});
  }

  DuneBuggy::DuneBuggy(ezecs::State &state, Scene_ &scene, glm::mat4 &transform) : state(&state), scene(&scene) {
    chassis = std::shared_ptr<MeshObject_> (
        new MeshObject_("assets/models/pyramid_bottom.dae", "assets/textures/pyramid_bottom.png", transform));
    chassisId = chassis->getId();

    glm::mat4 ident;
    std::vector<float> chassisVerts = {
        2.0f,  2.0f, -0.4f,
        2.0f, -2.0f, -0.4f,
        -2.0f, -2.0f, -0.4f,
        -2.0f,  2.0f, -0.4f,
        2.0f,  2.0f,  0.4f,
        2.0f, -2.0f,  0.4f,
        -2.0f, -2.0f,  0.4f,
        -2.0f,  2.0f,  0.4f,
    };
    state.add_TransformFunction(chassisId, DELEGATE_NOCLASS(chassisScalar));
    state.add_Physics(chassisId, 50.f, &chassisVerts, Physics::MESH);
    Physics *physics;
    state.get_Physics(chassisId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    state.add_TrackControls(chassisId);
    TrackControls *trackControls;
    state.get_TrackControls(chassisId, &trackControls);
    trackControls->tuning.m_suspensionStiffness = 16.f;     // 5.88f
    trackControls->tuning.m_suspensionCompression = 0.8f;   // 0.83f
    trackControls->tuning.m_suspensionDamping = 1.0f;       // 0.88f
    trackControls->tuning.m_maxSuspensionTravelCm = 60.f;   // 500.f
    trackControls->tuning.m_frictionSlip  = 100.f;          // 10.5f
    trackControls->tuning.m_maxSuspensionForce  = 20000.f;  // 6000.f
    btVector3 wheelConnectionPoints[4] {
        {-2.f,  2.0f, -0.4f},
        { 2.f,  2.0f, -0.4f},
        {-2.f, -2.0f, -0.4f},
        { 2.f, -2.0f, -0.4f}
    };
    for (int i = 0; i < 4; ++i) {
      wheels.push_back(std::shared_ptr<MeshObject_>(
          new MeshObject_("assets/models/sphere.dae", "assets/textures/thrusters.png", ident)));
      entityId wheelId = wheels.back()->getId();
      WheelInitInfo wheelInitInfo{
          {                         // WheelInfo struct - this part of the wheelInitInfo will persist.
              chassisId,                    // id of wheel's parent entity (chassis)
              wheelId,                      // wheel's own id (used for removal upon chassis deletion)
              -1,                           // bullet's index for this wheel (will be set correctly when wheel is added)
              wheelConnectionPoints[i].x()  // x component of wheel's chassis-space connection point
          },
          wheelConnectionPoints[i], // connection point
          {0.f, 0.f, -1.f},         // direction
          {1.f, 0.f, 0.f},          // axle
          0.3f,                     // suspension rest length
          WHEEL_RADIUS,             // wheel radius
          false                     // is front wheel
      };
      state.add_Physics(wheelId, 10.f, &wheelInitInfo, Physics::WHEEL);
      state.add_TransformFunction(wheelId, DELEGATE_NOCLASS(wheelScaler));
    }
    addToScene();
  }
  void DuneBuggy::addToScene() {
    scene->addObject(chassis);
    for (auto wheel : wheels) {
      scene->addObject(wheel);
    }
  }
  void DuneBuggy::tip() {
    Physics *physics;
    state->get_Physics(chassis->getId(), &physics);
    physics->rigidBody->applyImpulse({0.f, 0.f, 100.f}, {1.f, 0.f, 0.f});
  }
}
#pragma clang diagnostic pop