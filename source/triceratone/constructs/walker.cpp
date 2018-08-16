
#include "walker.hpp"

#include "serialization.hpp"
#include "cylinderMath.hpp"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  namespace Walker {

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

    const TransformFunctionDescriptor & getBodyTransFuncDesc() {
      static TransformFunctionDescriptor bodyTransFuncDesc;
      if ( ! bodyTransFuncDesc.registrationId) {
        bodyTransFuncDesc.func = RTU_FUNC_DLGT(bodyVisualTransform);
        rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", bodyTransFuncDesc);
      }
      return bodyTransFuncDesc;
    }

    static entityId mostRecentlyCreated = 0;
    static entityId mostRecentVisual = 0;

    entityId create(State &state, const glm::mat4 &transform, const entityId &id) {
      // The order of entity creation matters here, unfortunately, because it determines the order of the id's in
      // the Network System's registry, which determines the order in which the entities are sent to each new client.
      // And if the clients receive those in the wrong order, they can't build their scene trees correctly.

      glm::mat4 ident(1.f);

      entityId physicalId;
      state.createEntity(&physicalId);
      state.addPlacement(physicalId, transform);
      state.addPhysics(physicalId, 1.f, nullptr, Physics::CHARA);
      Physics* physics;
      state.getPhysics(physicalId, &physics);
      physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
      state.addNetworking(physicalId);
      state.addSceneNode(physicalId, 0);

      entityId gimbalId;
      state.createEntity(&gimbalId);
      state.addPlacement(gimbalId, ident);
      Placement *placement;
      state.getPlacement(gimbalId, &placement);
      placement->forceLocalRotationAndScale = true;
      state.addMouseControls(gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY, false);
      state.addNetworking(gimbalId);
      state.addSceneNode(gimbalId, physicalId);

      // Add the actual walking controls to the physical object
      state.addWalkControls(physicalId, gimbalId);

      state.createEntity(&mostRecentlyCreated);
      float back = 5.f;
      float tilt = 0.35f;
      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
      state.addPlacement(mostRecentlyCreated, camMat);
      state.addCamera(mostRecentlyCreated, settings::graphics::fovy, 0.1f, 10000.f);
      state.addNetworking(mostRecentlyCreated);
      state.addSceneNode(mostRecentlyCreated, gimbalId);

      state.createEntity(&mostRecentVisual);
      state.addPlacement(mostRecentVisual, ident);
      state.addTransformFunction(mostRecentVisual, getBodyTransFuncDesc().registrationId);
      state.addMesh(mostRecentVisual, "humanBean", "grass1024_00");
      state.addNetworking(mostRecentVisual);
      state.addSceneNode(mostRecentVisual, physicalId);

      return mostRecentlyCreated; // All the id's can be derived from camId through the SceneNode component's parentId.
    }
    void broadcastLatest(ezecs::State &state, EntityComponentSystemInterface &ecs) {
      SceneNode *sceneNode;
      state.getSceneNode(mostRecentlyCreated, &sceneNode);
      entityId gimbalId = sceneNode->parentId;
      state.getSceneNode(gimbalId, &sceneNode);
      entityId ctrlId = sceneNode->parentId;
      ecs.broadcastManualEntity(ctrlId);
      ecs.broadcastManualEntity(gimbalId);
      ecs.broadcastManualEntity(mostRecentlyCreated);
      ecs.broadcastManualEntity(mostRecentVisual);
    }
    void switchTo(State &state, const entityId & id) {
      publish<entityId>("switch_to_camera", id);
      SceneNode * sceneNode;
      state.getSceneNode(id, &sceneNode);
      publish<entityId>("switch_to_mouse_controls", sceneNode->parentId);
      state.getSceneNode(sceneNode->parentId, &sceneNode);
      publish<entityId>("switch_to_walking_controls", sceneNode->parentId);
    }
  }
}
