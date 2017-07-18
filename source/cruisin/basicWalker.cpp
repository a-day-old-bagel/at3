
#include "basicWalker.h"

#define HUMAN_HEIGHT 1.83f
#define HUMAN_WIDTH 0.5f
#define HUMAN_DEPTH 0.3f

using namespace ezecs;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

namespace at3 {

  static glm::mat4 bodyScaler(const glm::mat4& transformIn, uint32_t time) {
    return glm::scale(glm::translate(glm::mat4(), {0.f, 0.f, -HUMAN_HEIGHT * 0.165f}),
                      {HUMAN_WIDTH * 0.5f, HUMAN_DEPTH * 0.5f, HUMAN_HEIGHT * 0.5f});
  }

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
    state.add_TransformFunction(bodyId, RTU_FUNC_DLGT(bodyScaler));
    mpPhysicsBody->addChild(mpVisualBody);

    mpCamera = std::make_shared<ThirdPersonCamera_> (0.f, 5.f, (float)M_PI * 0.5f);
    mpPhysicsBody->addChild(mpCamera->mpCamGimbal, SceneObject_::TRANSLATION_ONLY);

    addToScene();
  }
  std::shared_ptr<PerspectiveCamera_> BasicWalker::getCamPtr() {
    return mpCamera->mpCamera;
  }
  void BasicWalker::addToScene() {
    mpScene->addObject(mpPhysicsBody);
  }
}
#pragma clang diagnostic pop
