
#include "pyramid.hpp"
#include "topics.hpp"
#include "cylinderMath.hpp"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  static glm::mat4 pyrTopRotate(
      const glm::mat4& transIn,
      const glm::mat4& absTransIn,
      uint32_t time,
      TransFuncEcsContext *ctxt = nullptr) {
    return transIn * glm::rotate(glm::mat4(1.f), time * 0.002f, {0.f, 0.f, 1.f});
  }

  static glm::mat4 pyrFireWiggle(
      const glm::mat4& transIn,
      const glm::mat4& absTransIn,
      uint32_t time,
      TransFuncEcsContext *ctxt = nullptr) {
    if (ctxt) {
      auto state = (State *) ctxt->ecs;
      // make the fire look bigger depending on how much the pyramid's thruster is being used
      SceneNode *sceneNode;
      state->getSceneNode(ctxt->id, &sceneNode);
      PyramidControls *controls;
      state->getPyramidControls(sceneNode->parentId, &controls);
      float pyrFireSize, pyrFireFlicker;
      switch (controls->engineActivationLevel) {
        case 2: {
          pyrFireSize = 3.f;
          pyrFireFlicker = 2.f;
        } break;
        case 1: {
          pyrFireSize = 1.5f;
          pyrFireFlicker = 1.f;
        } break;
        case 0:
        default: {
          pyrFireSize = 0.3f;
          pyrFireFlicker = 0.15f;
        }
      }
      return transIn * glm::scale(glm::mat4(1.f), {1.f, 1.f, pyrFireSize + pyrFireFlicker * sin(time * 0.1f)});
    } else {
      return transIn;
    }
  }

  const TransformFunctionDescriptor & Pyramid::getTopTransFuncDesc() {
    static TransformFunctionDescriptor topTransFuncDesc;
    if ( ! topTransFuncDesc.registrationId) {
      topTransFuncDesc.func = RTU_FUNC_DLGT(pyrTopRotate);
      rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", topTransFuncDesc);
    }
    return topTransFuncDesc;
  }

  const TransformFunctionDescriptor & Pyramid::getFireTransFuncDesc() {
    static TransformFunctionDescriptor fireTransFuncDesc;
    if ( ! fireTransFuncDesc.registrationId) {
      fireTransFuncDesc.func = RTU_FUNC_DLGT(pyrFireWiggle);
      rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", fireTransFuncDesc);
    }
    return fireTransFuncDesc;
  }

  Pyramid::Pyramid(ezecs::State &state, std::shared_ptr<EntityComponentSystemInterface> ecs, glm::mat4 &transform)
          : state(&state), ecs(ecs) {

    glm::mat4 ident(1.f);
    glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                                 (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});

    state.createEntity(&bottomId);
    state.addPlacement(bottomId, transform);
    state.addMesh(bottomId, "pyramid_bottom", "pyramid_bottom");

    state.createEntity(&topId);
    state.addPlacement(topId, ident);
    state.addTransformFunction(topId, getTopTransFuncDesc().registrationId);
    state.addMesh(topId, "pyramid_top", "pyramid_top");

    state.createEntity(&thrustersId);
    state.addPlacement(thrustersId, ident);
    state.addMesh(thrustersId, "pyramid_thrusters", "pyramid_thrusters");

    state.createEntity(&fireId);
    state.addPlacement(fireId, pyrFirMat);
    state.addTransformFunction(fireId, getFireTransFuncDesc().registrationId);
    state.addMesh(fireId, "pyramid_thruster_flames", "pyramid_flames");

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

    std::shared_ptr<std::vector<float>> hullVerts = std::make_shared<std::vector<float>>( std::vector<float> {
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
    });
    state.addPhysics(bottomId, 100.f, hullVerts, Physics::DYNAMIC_CONVEX_MESH);
    state.addPyramidControls(bottomId, camGimbalId);

    Physics* physics;
    state.getPhysics(bottomId, &physics);
    physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
    physics->rigidBody->setDamping(0.f, 0.8f);

    // The slow stuff (CCD)
    physics->rigidBody->setCcdMotionThreshold(1);
    physics->rigidBody->setCcdSweptSphereRadius(0.2f);

    addToScene();
  }
  void Pyramid::addToScene() {
    state->addSceneNode(bottomId, 0);
    state->addSceneNode(fireId, bottomId);
    state->addSceneNode(thrustersId, bottomId);
    state->addSceneNode(topId, bottomId);
    state->addSceneNode(camGimbalId, bottomId);
    state->addSceneNode(camId, camGimbalId);
  }
  void Pyramid::makeActiveControl(void *nothing) {
    publish<entityId>("switch_to_pyramid_controls", bottomId);
    publish<entityId>("switch_to_mouse_controls", camGimbalId);
    publish<entityId>("switch_to_camera", camId);
  }
}
#pragma clang diagnostic pop
