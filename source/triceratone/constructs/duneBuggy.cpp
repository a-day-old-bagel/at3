
#include "duneBuggy.hpp"
#include "math.hpp"
#include "topics.hpp"
#include "cylinderMath.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  static glm::mat4 wheelScaler(
      const glm::mat4& transIn,
      const glm::mat4& absTransIn,
      uint32_t time,
      TransFuncEcsContext *ctxt = nullptr) {
    return glm::scale(transIn, glm::vec3(WHEEL_RADIUS * 2.f));
  }

  const TransformFunctionDescriptor & DuneBuggy::getWheelTransFuncDesc() {
    static TransformFunctionDescriptor wheelTransFuncDesc;
    if ( ! wheelTransFuncDesc.registrationId) {
      wheelTransFuncDesc.func = RTU_FUNC_DLGT(wheelScaler);
      rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", wheelTransFuncDesc);
    }
    return wheelTransFuncDesc;
  }

  DuneBuggy::DuneBuggy(ezecs::State &state, glm::mat4 &transform) : state(&state) {

    state.createEntity(&chassisId);
    state.add_Placement(chassisId, transform);
    state.add_Mesh(chassisId, "chassis", "cliff1024_00");

    glm::mat4 ident(1.f);
    std::shared_ptr<std::vector<float>> chassisVerts = std::make_shared<std::vector<float>>(std::vector<float> {
         1.7f,  1.7f, -0.4f,
         1.7f, -1.7f, -0.4f,
        -1.7f, -1.7f, -0.4f,
        -1.7f,  1.7f, -0.4f,
         2.1f,  2.1f,  0.4f,
         2.1f, -2.1f,  0.4f,
        -2.1f, -2.1f,  0.4f,
        -2.1f,  2.1f,  0.4f
    });
    state.add_Physics(chassisId, 50.f, chassisVerts, Physics::DYNAMIC_CONVEX_MESH);
    Physics *physics;
    state.get_Physics(chassisId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);

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
      entityId wheelId;
      state.createEntity(&wheelId);
      state.add_Placement(wheelId, ident);
      state.add_Mesh(wheelId, "wheel", "tire");
      wheels.emplace_back(wheelId);

      std::shared_ptr<WheelInitInfo> wheelInitInfo = std::make_shared<WheelInitInfo>( WheelInitInfo {
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
      });
      state.add_Physics(wheelId, 10.f, wheelInitInfo, Physics::WHEEL);

      state.add_TransformFunction(wheelId, getWheelTransFuncDesc().registrationId);
    }

    state.createEntity(&camId);
    float back = 10.f;
    float tilt = 0.35f;
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
    state.add_Placement(camId, camMat);
    state.add_Camera(camId, settings::graphics::fovy, 0.1f, 10000.f);

    state.createEntity(&camGimbalId);
    state.add_Placement(camGimbalId, ident);
    Placement *placement;
    state.get_Placement(camGimbalId, &placement);
    placement->forceLocalRotationAndScale = true;
    state.add_MouseControls(camGimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);

    addToScene();
  }
  void DuneBuggy::addToScene() {
    state->add_SceneNode(chassisId, 0);
    state->add_SceneNode(camGimbalId, chassisId);
    state->add_SceneNode(camId, camGimbalId);
    for (const auto & wheel : wheels) {
      state->add_SceneNode(wheel, 0);
    }
  }

  void DuneBuggy::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_track_controls", chassisId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<entityId>("switch_to_camera", camId);
  }

}
#pragma clang diagnostic pop
