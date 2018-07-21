
#include "walker.hpp"
#include "topics.hpp"
#include "math.hpp"
#include "cylinderMath.hpp"

using namespace ezecs;
using namespace rtu::topics;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

namespace at3 {

  Walker::Walker(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context,
                               Scene &scene, glm::mat4 &transform)
      : state(&state), scene(&scene) {

    glm::mat4 ident(1.f);
    glm::mat4 rotatedTransform = glm::rotate(transform, (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f));

    camera = std::make_shared<ThirdPersonCamera> (5.f, 0.3f);
    camGimbalId = camera->gimbal->getId();

    physicsBody = std::make_shared<Object>();
    entityId physicalId = physicsBody->getId();
    state.add_Placement(physicalId, transform);
    state.add_Physics(physicalId, 1.f, nullptr, Physics::CHARA);
    Physics* physics;
    state.get_Physics(physicalId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    state.add_PlayerControls(physicalId, camGimbalId);
    ctrlId = physicalId;

    visualBody = std::make_shared<Mesh>(context, "humanBean", "grass1024_00", ident);

    entityId bodyId = visualBody->getId();
    state.add_TransformFunction(bodyId, RTU_MTHD_DLGT(&Walker::bodyVisualTransform, this));
    physicsBody->addChild(visualBody);

    camera->anchorTo(visualBody);

    addToScene();
  }
  std::shared_ptr<PerspectiveCamera> Walker::getCamPtr() {
    return camera->actual;
  }
  void Walker::addToScene() {
    scene->addObject(physicsBody);
  }

  glm::mat4 Walker::bodyVisualTransform(const glm::mat4 &transIn, const glm::mat4& absTransIn, uint32_t time) {
//    return glm::scale(transIn, glm::vec3(.4f, .4f, .4f));

    MouseControls *mouseControls;
    state->get_MouseControls(camera->gimbal->getId(), &mouseControls);
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
    publish<entityId>("switch_to_walking_controls", ctrlId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<std::shared_ptr<PerspectiveCamera>>("set_primary_camera", camera->actual);
  }
}
#pragma clang diagnostic pop
