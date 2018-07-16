
#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include "pyramid.hpp"
#include "topics.hpp"

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

static glm::mat4 cylindricalCameraRotate(const glm::mat4 &transIn, const glm::mat4& absTransIn, uint32_t time) {
//  float xPos = absTransIn[3][0];
//  float zPos = absTransIn[3][2];
//  float cylAngle = atan2f(-xPos, -zPos);
//  return transIn * glm::rotate(cylAngle, glm::vec3(0.f, 1.f, 0.f));

//  printf("%s\n\n", glm::to_string(absTransIn).c_str());
//  glm::vec3 pos = {absTransIn[3][0], absTransIn[3][1], absTransIn[3][2]};
////  printf("%.1f %.1f %.1f\n", pos.x, pos.y, pos.z);
//  if (pos.x || pos.z) {
//    printf("%f %f\n", pos.x, pos.z);
//    glm::vec3 cylUp = glm::normalize(glm::vec3(-pos.x, 0, -pos.z));
//    glm::vec3 at = glm::mat3(absTransIn) * glm::vec3(0, 1, 0);
////  printf("%.1f %.1f %.1f\n", at.x, at.y, at.z);
//    glm::mat4 final = glm::lookAt(glm::vec3(0, 0, 0), at, cylUp);
////  printf("%s\n\n", glm::to_string(final).c_str());
////  glm::vec4 newAt = final * glm::vec4(0, 1, 0, 1);
////  printf("%.1f %.1f %.1f\n", newAt.x, newAt.y, newAt.z);
//////  return glm::lookAt(glm::vec3(), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
//    return glm::lookAt(glm::vec3(), glm::vec3(0, 1, 0), cylUp);
//  } else {
//    printf("foo\n");
//    return glm::mat4(1.f);
//  }

//  return transIn;
  return glm::mat4(1.f);
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
    state.add_PyramidControls(bottomId);

    Physics* physics;
    state.get_Physics(bottomId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setDamping(0.1f, 0.8f);

    // The slow stuff (CCD)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);

    entityId topId = top->getId();
    state.add_TransformFunction(topId, RTU_FUNC_DLGT(pyrTopRotate));

    entityId fireId = fire->getId();
    state.add_TransformFunction(fireId, RTU_FUNC_DLGT(pyrFireWiggle));

//    camera = std::make_shared<ThirdPersonCamera> (0.f, 5.f, 0.f, RTU_FUNC_DLGT(cylindricalCameraRotate));
    camera = std::make_shared<ThirdPersonCamera> (0.f, 5.f, 0.f);
//    camera = std::make_shared<ThirdPersonCamera> (0.f, 0.f, 0.f);
    camera->anchorTo(base);

    ctrlId = bottomId;
    camGimbalId = camera->gimbal->getId();

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
    if (controls->force.z > 0) {
      pyrFireSize = 1.5f;
      pyrFireScale = 1.f;
    } else {
      pyrFireSize = 0.3f;
      pyrFireScale = 0.15f;
    }
  }

  void Pyramid::spawnSphere() {
    Placement *source;
    state->get_Placement(ctrlId, &source);
    Physics *sourcePhysics;
    state->get_Physics(ctrlId, &sourcePhysics);

    glm::mat4 sourceMat = glm::translate(source->mat, {0.f, 0.f, 3.f});
    spheres.push_back(std::make_shared<Mesh>(vkc, "sphere", sourceMat));
    float sphereRadius = 1.0f;
    state->add_Physics(spheres.back()->getId(), 5.f, &sphereRadius, Physics::SPHERE);
    Physics *physics;
    state->get_Physics(spheres.back()->getId(), &physics);
    physics->rigidBody->setLinearVelocity(sourcePhysics->rigidBody->getLinearVelocity() );//+ btVector3(0.f, 0.f, 1.f));
    scene->addObject(spheres.back());
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
