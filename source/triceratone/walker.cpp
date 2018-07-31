
#include "walker.hpp"
#include "topics.hpp"
#include "math.hpp"
#include "cylinderMath.hpp"

using namespace ezecs;
using namespace rtu::topics;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

namespace at3 {

  Walker::Walker(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context, glm::mat4 &transform)
      : state(&state){

    glm::mat4 ident(1.f);

    state.createEntity(&camId);
    float back = 5.f;
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

    state.createEntity(&physicalId);
    state.add_Placement(physicalId, transform);
    state.add_Physics(physicalId, 1.f, nullptr, Physics::CHARA);
    Physics* physics;
    state.get_Physics(physicalId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    state.add_PlayerControls(physicalId, camGimbalId);

    state.createEntity(&visualId);
    state.add_Placement(visualId, ident);
    state.add_TransformFunction(visualId, RTU_MTHD_DLGT(&Walker::bodyVisualTransform, this));
    context->registerMeshInstance(visualId, "humanBean", "grass1024_00");

    addToScene();
  }

  void Walker::addToScene() {
    state->add_SceneNode(physicalId, 0);
    state->add_SceneNode(visualId, physicalId);
    state->add_SceneNode(camGimbalId, visualId);
    state->add_SceneNode(camId, camGimbalId);
  }

  glm::mat4 Walker::bodyVisualTransform(const glm::mat4 &transIn, const glm::mat4& absTransIn, uint32_t time) {

    MouseControls *mouseControls;
    state->get_MouseControls(camGimbalId, &mouseControls);
    glm::vec3 pos = {absTransIn[3][0], absTransIn[3][1], absTransIn[3][2]};
    glm::mat4 rot = glm::mat4(getCylStandingRot(pos, -halfPi, mouseControls->yaw));
    return transIn * rot;

//    Placement *camPlacement;
//    state->get_Placement(camera->gimbal->getId(), &camPlacement);
//    PlayerControls *controls;
//    state->get_PlayerControls(physicsBody->getId(), &controls);
//    float correction = 0.f;
//    if (controls->isGrounded) {
//      correction = controls->equilibriumOffset;
//      float correctionThresholdHigh = -1.f;
//      float correctionThresholdLow = 0.f;
//      if (correction < correctionThresholdLow && correction > correctionThresholdHigh) {
//        float range = correctionThresholdHigh - correctionThresholdLow;
//        correction *= pow((correctionThresholdHigh - correction) / range, 2);
//      }
//    }
//    return transIn * glm::scale(
//        glm::rotate(
//            glm::translate(
//                glm::mat4(1.f),
//                {0.f, 0.f, -HUMAN_HEIGHT * 0.165f + correction}),
//            camPlacement->getHorizRot(), glm::vec3(0.0f, 0.0f, 1.0f)),
//        {HUMAN_WIDTH * 0.5f, HUMAN_DEPTH * 0.5f, HUMAN_HEIGHT * 0.5f});
  }

  void Walker::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_walking_controls", physicalId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<entityId>("switch_to_camera", camId);
  }
}
#pragma clang diagnostic pop
