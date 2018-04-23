
#include <glm/gtc/matrix_transform.hpp>
#include "pyramidVk.h"
#include "topics.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;
using namespace rtu::topics;

static float pyrFireSize = 0.3f, pyrFireScale = 0.15f;
static glm::mat4 pyrFireWiggle(const glm::mat4& transformIn, uint32_t time) {
  return glm::scale(glm::mat4(), { 1.f, 1.f, pyrFireSize + pyrFireScale * sin(time * 0.1f) });
}

static glm::mat4 pyrTopRotate(const glm::mat4& transformIn, uint32_t time) {
  return glm::rotate(glm::mat4(), time * 0.002f, {0.f, 0.f, 1.f});
}

namespace at3 {

  PyramidVk::PyramidVk(ezecs::State &state, VulkanContext<EntityComponentSystemInterface> *context, Scene_ &scene,
                       glm::mat4 &transform) : mpState(&state), mpScene(&scene), vkc(context) {
    glm::mat4 ident;
    glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                                 (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});

    mpBase = std::make_shared<MeshObjectVk_> (context,
        "pyramid_bottom", "pyramid_bottom.png", transform, MeshObjectVk_::SUNNY);
    mpTop = std::make_shared<MeshObjectVk_> (context,
        "pyramid_top", "pyramid_top_new.png", ident, MeshObjectVk_::SUNNY);
    mpThrusters = std::make_shared<MeshObjectVk_> (context,
        "pyramid_thrusters", "thrusters.png", ident, MeshObjectVk_::SUNNY);
    mpFire = std::make_shared<MeshObjectVk_> (context,
        "pyramid_thruster_flames", "pyramid_flames.png",
        pyrFirMat, MeshObjectVk_::FULLBRIGHT);

    entityId bottomId = mpBase->getId();
    std::vector<float> hullVerts = {
        1.0f,  1.0f, -0.4f,
        1.0f, -1.0f, -0.4f,
        0.f ,  0.f ,  1.7f,
        1.0f,  1.0f, -0.4f,
        -1.0f,  1.0f, -0.4f,
        0.f ,  0.f ,  1.7f,
        -1.0f, -1.0f, -0.4f,
        -1.0f,  1.0f, -0.4f,
        1.0f, -1.0f, -0.4f,
        -1.0f, -1.0f, -0.4f,
    };
    state.add_Physics(bottomId, 100.f, &hullVerts, Physics::MESH);
    state.add_PyramidControls(bottomId);

    Physics* physics;
    state.get_Physics(bottomId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setDamping(0.1f, 0.8f);

    // The slow stuff (CCD)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);

    entityId topId = mpTop->getId();
    state.add_TransformFunction(topId, RTU_FUNC_DLGT(pyrTopRotate));

    entityId fireId = mpFire->getId();
    state.add_TransformFunction(fireId, RTU_FUNC_DLGT(pyrFireWiggle));

    mpCamera = std::make_shared<ThirdPersonCamera_> (2.5f, 5.f, (float)M_PI * 0.5f);
    mpBase->addChild(mpCamera->mpCamGimbal, SceneObject_::TRANSLATION_ONLY);

    ctrlId = bottomId;
    camGimbalId = mpCamera->mpCamGimbal->getId();

    addToScene();
  }
  void PyramidVk::addToScene() {
    mpScene->addObject(mpBase);
    mpBase->addChild(mpTop);
    mpBase->addChild(mpThrusters);
    mpBase->addChild(mpFire);
  }
  void PyramidVk::resizeFire() {
    // make the fire look big if the pyramid is thrusting upwards
    PyramidControls* controls;
    mpState->get_PyramidControls(mpBase->getId(), &controls);
    if (controls->force.z > 0) {
      pyrFireSize = 1.5f;
      pyrFireScale = 1.f;
    } else {
      pyrFireSize = 0.3f;
      pyrFireScale = 0.15f;
    }
  }

  void PyramidVk::spawnSphere() {
    Placement *source;
    mpState->get_Placement(mpBase->getId(), &source);
    glm::mat4 sourceMat = glm::translate(source->mat, {0.f, 0.f, 3.f});
    mvpSpheres.push_back(std::shared_ptr<MeshObjectVk_>(
        new MeshObjectVk_(vkc, "sphere", "pyramid_flames.png",
                        sourceMat, MeshObjectVk_::FULLBRIGHT)));
    float sphereRadius = 1.0f;
    mpState->add_Physics(mvpSpheres.back()->getId(), 5.f, &sphereRadius, Physics::SPHERE);
    Physics *physics;
    mpState->get_Physics(mvpSpheres.back()->getId(), &physics);
    physics->rigidBody->applyCentralImpulse({0.f, 0.f, 1.f});
    mpScene->addObject(mvpSpheres.back());
  }

  std::shared_ptr<PerspectiveCamera_> PyramidVk::getCamPtr() {
    return mpCamera->mpCamera;
  }

  void PyramidVk::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_pyramid_controls", ctrlId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<std::shared_ptr<PerspectiveCamera_>>("set_primary_camera", mpCamera->mpCamera);
  }
}
#pragma clang diagnostic pop
