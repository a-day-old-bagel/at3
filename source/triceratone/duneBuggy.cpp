
#include "configuration.h"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>
#include "duneBuggy.h"
#include "topics.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  static glm::mat4 wheelScaler(const glm::mat4& transformIn, uint32_t time) {
    return glm::scale(glm::mat4(1.f), {0.5f, WHEEL_RADIUS, WHEEL_RADIUS});
  }
  static glm::mat4 chassisScalar(const glm::mat4& transformIn, uint32_t time) {
    return glm::rotate(glm::scale(glm::mat4(1.f), {1.8f, 2.8f, 1.f}), (float)M_PI, glm::vec3(1.f, 0.f, 0.f));
  }

  DuneBuggy::DuneBuggy(ezecs::State &state, Scene_ &scene, glm::mat4 &transform) : state(&state), scene(&scene) {

    chassis = std::make_shared<MeshObject_>("assets/models/pyramid_bottom.dae", transform);
//    mpChassis = std::make_shared<MeshObject_>("assets/models/jeep.dae", transform);

    ezecs::entityId chassisId = chassis->getId();

    glm::mat4 ident(1.f);
    std::vector<float> chassisVerts = {
        1.7f,  1.7f, -0.4f,
        1.7f, -1.7f, -0.4f,
        -1.7f, -1.7f, -0.4f,
        -1.7f,  1.7f, -0.4f,
        2.1f,  2.1f,  0.4f,
        2.1f, -2.1f,  0.4f,
        -2.1f, -2.1f,  0.4f,
        -2.1f,  2.1f,  0.4f,
    };
    state.add_TransformFunction(chassisId, RTU_FUNC_DLGT(chassisScalar));
    state.add_Physics(chassisId, 50.f, &chassisVerts, Physics::MESH);
    Physics *physics;
    state.get_Physics(chassisId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setFriction(0.2f);

    // The slow stuff (CCD)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);

    state.add_TrackControls(chassisId);
    TrackControls *trackControls;
    state.get_TrackControls(chassisId, &trackControls);
    trackControls->tuning.m_suspensionStiffness = 16.f;     // 5.88f
    trackControls->tuning.m_suspensionCompression = 0.8f;   // 0.83f
    trackControls->tuning.m_suspensionDamping = 1.0f;       // 0.88f
    trackControls->tuning.m_maxSuspensionTravelCm = 80.f;   // 500.f
    trackControls->tuning.m_frictionSlip  = 40.f;           // 10.5f
    trackControls->tuning.m_maxSuspensionForce  = 20000.f;  // 6000.f
    btVector3 wheelConnectionPoints[4] {
        {-1.9f,  1.9f, 0.f},
        { 1.9f,  1.9f, 0.f},
        {-1.9f, -1.9f, 0.f},
        { 1.9f, -1.9f, 0.f}
    };
    for (int i = 0; i < 4; ++i) {
      wheels.push_back(std::make_shared<MeshObject_>("assets/models/sphere.dae", "assets/textures/thrusters.png",
                          ident, MeshObject_::SUNNY));
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
          0.4f,                     // suspension rest length
          WHEEL_RADIUS,             // wheel radius
          false                     // is front wheel
      };
      state.add_Physics(wheelId, 10.f, &wheelInitInfo, Physics::WHEEL);
      state.add_TransformFunction(wheelId, RTU_FUNC_DLGT(wheelScaler));
    }

    camera = std::make_shared<ThirdPersonCamera_>(2.f, 7.f, (float)M_PI * 0.5f);
    chassis->addChild(camera->mpCamGimbal, SceneObject_::TRANSLATION_ONLY);

    ctrlId = chassisId;
    camGimbalId = camera->mpCamGimbal->getId();

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

  std::shared_ptr<PerspectiveCamera_> DuneBuggy::getCamPtr() {
    return camera->mpCamera;
  }

  void DuneBuggy::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_track_controls", ctrlId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<std::shared_ptr<PerspectiveCamera_>>("set_primary_camera", camera->mpCamera);
  }

}
#pragma clang diagnostic pop
