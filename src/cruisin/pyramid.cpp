//
// Created by volundr on 4/27/17.
//

#include <glm/gtc/matrix_transform.hpp>
#include "pyramid.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;

static float pyrFireSize = 0.3f, pyrFireScale = 0.15f;
static glm::mat4 pyrFireWiggle(const glm::mat4& transformIn, uint32_t time) {
  return glm::scale(glm::mat4(), { 1.f, 1.f, pyrFireSize + pyrFireScale * sin(time * 0.1f) });
}

static glm::mat4 pyrTopRotate(const glm::mat4& transformIn, uint32_t time) {
  return glm::rotate(glm::mat4(), time * 0.002f, {0.f, 0.f, 1.f});
}

namespace at3 {

  Pyramid::Pyramid(ezecs::State &state, Scene_ &scene, glm::mat4 &transform) : state(&state), scene(&scene) {
    glm::mat4 ident;
    glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                                 (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});

    base = std::shared_ptr<MeshObject_> (
        new MeshObject_("assets/models/pyramid_bottom.dae", "assets/textures/pyramid_bottom.png", transform));
    top = std::shared_ptr<MeshObject_> (
        new MeshObject_("assets/models/pyramid_top.dae", "assets/textures/pyramid_top_new.png", ident));
    thrusters = std::shared_ptr<MeshObject_> (
        new MeshObject_("assets/models/pyramid_thrusters.dae", "assets/textures/thrusters.png", ident));
    fire = std::shared_ptr<MeshObject_> (
        new MeshObject_("assets/models/pyramid_thruster_flames.dae", "assets/textures/pyramid_flames.png", pyrFirMat));

    entityId bottomId = base->getId();
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
    state.add_PyramidControls(bottomId, PyramidControls::ROTATE_ABOUT_Z);

    Physics* physics;
    state.get_Physics(bottomId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setDamping(0.1f, 0.8f);

    entityId topId = top->getId();
    state.add_TransformFunction(topId, DELEGATE_NOCLASS(pyrTopRotate));

    entityId fireId = fire->getId();
    state.add_TransformFunction(fireId, DELEGATE_NOCLASS(pyrFireWiggle));

    addToScene();
  }
  void Pyramid::addToScene() {
    scene->addObject(base);
    base->addChild(top);
    base->addChild(thrusters);
    base->addChild(fire);
  }
  void Pyramid::resizeFire() {
    // make the fire look big if the pyramid is thrusting upwards
    PyramidControls* controls;
    state->get_PyramidControls(base->getId(), &controls);
    if (controls->accel.z > 0) {
      pyrFireSize = 1.5f;
      pyrFireScale = 1.f;
    } else {
      pyrFireSize = 0.3f;
      pyrFireScale = 0.15f;
    }
  }
  void Pyramid::spawnSphere() {
    Placement *source;
    state->get_Placement(base->getId(), &source);
    glm::mat4 sourceMat = glm::translate(source->mat, {0.f, 0.f, 3.f});
    spheres.push_back(std::shared_ptr<MeshObject_>(
        new MeshObject_("assets/models/sphere.dae", "assets/textures/pyramid_flames.png", sourceMat)));
    float sphereRadius = 1.0f;
    state->add_Physics(spheres.back()->getId(), 5.f, &sphereRadius, Physics::SPHERE);
    Physics *physics;
    state->get_Physics(spheres.back()->getId(), &physics);
    physics->rigidBody->applyCentralImpulse({0.f, 0.f, 1.f});
    state->add_SweeperTarget(spheres.back()->getId());
    scene->addObject(spheres.back());
  }
}
#pragma clang diagnostic pop