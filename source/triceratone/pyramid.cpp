
#include "pyramid.hpp"
#include "topics.hpp"
#include "cylinderMath.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;
using namespace rtu::topics;

static float pyrFireSize = 0.3f, pyrFireScale = 0.15f;
static glm::mat4 pyrFireWiggle(const glm::mat4& transIn, const glm::mat4& absTransIn, uint32_t time) {
  return transIn * glm::scale(glm::mat4(1.f), { 1.f, 1.f, pyrFireSize + pyrFireScale * sin(time * 0.1f) });
}

static glm::mat4 pyrTopRotate(const glm::mat4& transIn, const glm::mat4& absTransIn, uint32_t time) {
  return transIn * glm::rotate(glm::mat4(1.f), time * 0.002f, {0.f, 0.f, 1.f});
}

namespace at3 {

  Pyramid::Pyramid(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context, Scene &scene,
                       glm::mat4 &transform) : state(&state), scene(&scene), vkc(context) {

    glm::mat4 ident(1.f);
    glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                                 (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});

    base = std::make_shared<Mesh> (context, "pyramid_bottom", "pyramid_bottom", transform);
    top = std::make_shared<Mesh> (context, "pyramid_top", "pyramid_top", ident);
    thrusters = std::make_shared<Mesh> (context, "pyramid_thrusters", "pyramid_thrusters", ident);
    fire = std::make_shared<Mesh> (context, "pyramid_thruster_flames", "pyramid_flames", pyrFirMat,
        Mesh::FULLBRIGHT);

    camera = std::make_shared<ThirdPersonCamera> (5.f, .35f);
    camGimbalId = camera->gimbal->getId();

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
    state.add_Physics(bottomId, 100.f, &hullVerts, Physics::DYNAMIC_CONVEX_MESH);
    state.add_PyramidControls(bottomId, camGimbalId);

    Physics* physics;
    state.get_Physics(bottomId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setDamping(0.f, 0.8f);
    ctrlId = bottomId;

    // The slow stuff (CCD)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);

    entityId topId = top->getId();
    state.add_TransformFunction(topId, RTU_FUNC_DLGT(pyrTopRotate));

    entityId fireId = fire->getId();
    state.add_TransformFunction(fireId, RTU_FUNC_DLGT(pyrFireWiggle));

    camera->anchorTo(base);

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
    int level = 0;
    if (controls->turbo && controls->accel.y >= 0 && glm::length(controls->accel)) { ++level; }
    if (controls->accel.y > 0) { ++level; }
    switch (level) {
      case 2: {
        pyrFireSize = 3.f;
        pyrFireScale = 2.f;
      } break;
      case 1: {
        pyrFireSize = 1.5f;
        pyrFireScale = 1.f;
      } break;
      case 0:
      default: {
        pyrFireSize = 0.3f;
        pyrFireScale = 0.15f;
      }
    }
  }

  entityId Pyramid::spawnSphere(bool shoot) {
    Placement *source;
    state->get_Placement(ctrlId, &source);
    Physics *sourcePhysics;
    state->get_Physics(ctrlId, &sourcePhysics);

    glm::mat4 sourceMat = glm::translate(source->absMat, {0.f, 0.f, 3.f});
    spheres.push_back(std::make_shared<Mesh>(vkc, "sphere", sourceMat));
    float sphereRadius = 1.0f;
    state->add_Physics(spheres.back()->getId(), 5.f, &sphereRadius, Physics::SPHERE);
    Physics *physics;
    state->get_Physics(spheres.back()->getId(), &physics);
    physics->rigidBody->setLinearVelocity(sourcePhysics->rigidBody->getLinearVelocity() );

    if (shoot) {
      MouseControls *mouseControls;
      state->get_MouseControls(camGimbalId, &mouseControls);
      glm::mat3 tiltRot = glm::rotate(.35f , glm::vec3(1.0f, 0.0f, 0.0f));
      glm::mat3 rot = getCylStandingRot(source->getTranslation(true), mouseControls->pitch, mouseControls->yaw);
      glm::vec3 shootDir = rot * tiltRot * glm::vec3(0, 0, -1);
      btVector3 shot = btVector3(shootDir.x, shootDir.y, shootDir.z) * 1000.f;
      physics->rigidBody->applyCentralImpulse(shot);
    }

    scene->addObject(spheres.back());

    return spheres.back()->getId();
  }

  void Pyramid::dropSphere() {
    spawnSphere(false);
  }

  void Pyramid::shootSphere() {
    spawnSphere(true);
  }

  std::shared_ptr<PerspectiveCamera> Pyramid::getCamPtr() {
    return camera->actual;
  }

  void Pyramid::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_pyramid_controls", ctrlId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<std::shared_ptr<PerspectiveCamera>>("set_primary_camera", camera->actual);
  }
}
#pragma clang diagnostic pop
