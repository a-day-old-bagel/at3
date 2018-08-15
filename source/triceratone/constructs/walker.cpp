
#include "walker.hpp"
#include "topics.hpp"
#include "math.hpp"
#include "cylinderMath.hpp"

using namespace ezecs;
using namespace rtu::topics;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

namespace at3 {

  static glm::mat4 bodyVisualTransform(
      const glm::mat4& transIn,
      const glm::mat4& absTransIn,
      uint32_t time,
      TransFuncEcsContext *ctxt = nullptr) {
    if (ctxt) {
      auto state = (State*)ctxt->ecs;
      SceneNode *sceneNode;
      state->getSceneNode(ctxt->id, &sceneNode);
      WalkControls *walkControls;
      state->getWalkControls(sceneNode->parentId, &walkControls);
      MouseControls *mouseControls;
      state->getMouseControls(walkControls->mouseCtrlId, &mouseControls);
      glm::vec3 pos = {absTransIn[3][0], absTransIn[3][1], absTransIn[3][2]};
      glm::mat4 rot = glm::mat4(getCylStandingRot(pos, -halfPi, mouseControls->yaw));
      return transIn * rot;
    } else {
      return transIn;
    }

    // OLD stuff used to make the visible body snap to correct height (make spring motion invisible)
    // This wasn't updated when I made the shift to cylidrical gravity, and by now lots of things besides are outdated.
    ////    Placement *camPlacement;
    ////    state->getPlacement(camera->gimbal->getId(), &camPlacement);
    ////    WalkControls *controls;
    ////    state->getWalkControls(physicsBody->getId(), &controls);
    ////    float correction = 0.f;
    ////    if (controls->isGrounded) {
    ////      correction = controls->equilibriumOffset;
    ////      float correctionThresholdHigh = -1.f;
    ////      float correctionThresholdLow = 0.f;
    ////      if (correction < correctionThresholdLow && correction > correctionThresholdHigh) {
    ////        float range = correctionThresholdHigh - correctionThresholdLow;
    ////        correction *= pow((correctionThresholdHigh - correction) / range, 2);
    ////      }
    ////    }
    ////    return transIn * glm::scale(
    ////        glm::rotate(
    ////            glm::translate(
    ////                glm::mat4(1.f),
    ////                {0.f, 0.f, -HUMAN_HEIGHT * 0.165f + correction}),
    ////            camPlacement->getHorizRot(), glm::vec3(0.0f, 0.0f, 1.0f)),
    ////        {HUMAN_WIDTH * 0.5f, HUMAN_DEPTH * 0.5f, HUMAN_HEIGHT * 0.5f});
  }

  const TransformFunctionDescriptor & Walker::getBodyTransFuncDesc() {
    static TransformFunctionDescriptor bodyTransFuncDesc;
    if ( ! bodyTransFuncDesc.registrationId) {
      bodyTransFuncDesc.func = RTU_FUNC_DLGT(bodyVisualTransform);
      rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", bodyTransFuncDesc);
    }
    return bodyTransFuncDesc;
  }

  Walker::Walker(ezecs::State &state, glm::mat4 &transform) : state(&state){

    glm::mat4 ident(1.f);

    state.createEntity(&camId);
    float back = 5.f;
    float tilt = 0.35f;
    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
    state.addPlacement(camId, camMat);
    state.addCamera(camId, settings::graphics::fovy, 0.1f, 10000.f);

    state.createEntity(&camGimbalId);
    state.addPlacement(camGimbalId, ident);
    Placement *placement;
    state.getPlacement(camGimbalId, &placement);
    placement->forceLocalRotationAndScale = true;
    state.addMouseControls(camGimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);

    state.createEntity(&physicalId);
    state.addPlacement(physicalId, transform);
    state.addPhysics(physicalId, 1.f, nullptr, Physics::CHARA);
    Physics* physics;
    state.getPhysics(physicalId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    state.addWalkControls(physicalId, camGimbalId);

    state.createEntity(&visualId);
    state.addPlacement(visualId, ident);
    state.addTransformFunction(visualId, getBodyTransFuncDesc().registrationId);
    state.addMesh(visualId, "humanBean", "grass1024_00");

    addToScene();
  }

  void Walker::addToScene() {
    state->addSceneNode(physicalId, 0);
    state->addSceneNode(visualId, physicalId);
    state->addSceneNode(camGimbalId, visualId);
    state->addSceneNode(camId, camGimbalId);
  }

  void Walker::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_walking_controls", physicalId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<entityId>("switch_to_camera", camId);
  }
}
#pragma clang diagnostic pop
