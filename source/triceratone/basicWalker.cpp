
#include "basicWalker.h"
#include "topics.hpp"

#define HUMAN_HEIGHT 1.83f
#define HUMAN_WIDTH 0.5f
#define HUMAN_DEPTH 0.3f

using namespace ezecs;
using namespace rtu::topics;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

namespace at3 {

  BasicWalker::BasicWalker(ezecs::State &state, Scene_ &scene, glm::mat4 &transform)
      : mpState(&state), mpScene(&scene) {

    glm::mat4 ident;
    glm::mat4 rotatedTransform = glm::rotate(transform, (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f));

    mpPhysicsBody = std::make_shared<SceneObject_>();
    entityId physicalId = mpPhysicsBody->getId();
    state.add_Placement(physicalId, transform);
    state.add_Physics(physicalId, 1.f, NULL, Physics::CHARA);
    Physics* physics;
    state.get_Physics(physicalId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    state.add_PlayerControls(physicalId);

    mpVisualBody = std::make_shared<MeshObject_>("assets/models/sphere.dae",
                                           "assets/textures/pyramid_bottom.png",
                                           ident, MeshObject_::SUNNY);

    entityId bodyId = mpVisualBody->getId();
    state.add_TransformFunction(bodyId, RTU_MTHD_DLGT(&BasicWalker::bodyVisualTransform, this));
    mpPhysicsBody->addChild(mpVisualBody);

    mpCamera = std::make_shared<ThirdPersonCamera_> (0.f, 5.f, (float) M_PI * 0.5f);
    mpPhysicsBody->addChild(mpCamera->mpCamGimbal, SceneObject_::TRANSLATION_ONLY);

    ctrlId = physicalId;
    camGimbalId = mpCamera->mpCamGimbal->getId();

    addToScene();
  }
  std::shared_ptr<PerspectiveCamera_> BasicWalker::getCamPtr() {
    return mpCamera->mpCamera;
  }
  void BasicWalker::addToScene() {
    mpScene->addObject(mpPhysicsBody);
  }

  glm::mat4 BasicWalker::bodyVisualTransform(const glm::mat4 &transformIn, uint32_t time) {
    Placement *placement;
    mpState->get_Placement(mpCamera->mpCamGimbal->getId(), &placement);
    return glm::scale(
               glm::rotate(
                   glm::translate(
                       glm::mat4(),
                   {0.f, 0.f, -HUMAN_HEIGHT * 0.165f}),
               placement->getHorizRot(), glm::vec3(0.0f, 0.0f, 1.0f)),
           {HUMAN_WIDTH * 0.5f, HUMAN_DEPTH * 0.5f, HUMAN_HEIGHT * 0.5f});
  }

  void BasicWalker::makeActiveControl(void *nothing) {
    publish("switch_to_walking_controls", (void*)&ctrlId);
    publish("switch_to_mouse_controls", (void*)&camGimbalId);
    publish("set_primary_camera", (void*)&mpCamera->mpCamera);
  }
}
#pragma clang diagnostic pop