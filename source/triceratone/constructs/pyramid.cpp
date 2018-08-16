
#include "pyramid.hpp"
#include "serialization.hpp"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  namespace Pyramid {

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
    const TransformFunctionDescriptor & getTopTransFuncDesc() {
      static TransformFunctionDescriptor topTransFuncDesc;
      if ( ! topTransFuncDesc.registrationId) {
        topTransFuncDesc.func = RTU_FUNC_DLGT(pyrTopRotate);
        publish<TransformFunctionDescriptor>("register_transform_function", topTransFuncDesc);
      }
      return topTransFuncDesc;
    }
    const TransformFunctionDescriptor & getFireTransFuncDesc() {
      static TransformFunctionDescriptor fireTransFuncDesc;
      if ( ! fireTransFuncDesc.registrationId) {
        fireTransFuncDesc.func = RTU_FUNC_DLGT(pyrFireWiggle);
        publish<TransformFunctionDescriptor>("register_transform_function", fireTransFuncDesc);
      }
      return fireTransFuncDesc;
    }

    static entityId mostRecentlyCreated = 0;
    static entityId mostRecentTop = 0;
    static entityId mostRecentThrusters = 0;
    static entityId mostRecentFire = 0;

    entityId create(State &state, const glm::mat4 &transform) {
      // The order of entity creation matters here, unfortunately, because it determines the order of the id's in
      // the Network System's registry, which determines the order in which the entities are sent to each new client.
      // And if the clients receive those in the wrong order, they can't build their scene trees correctly.

      glm::mat4 ident(1.f);

      glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                       (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});

      entityId bottomId;
      state.createEntity(&bottomId);
      state.addPlacement(bottomId, transform);
      state.addMesh(bottomId, "pyramid_bottom", "pyramid_bottom");
      state.addNetworking(bottomId);
      state.addSceneNode(bottomId, 0);

      state.createEntity(&mostRecentTop);
      state.addPlacement(mostRecentTop, ident);
      state.addTransformFunction(mostRecentTop, getTopTransFuncDesc().registrationId);
      state.addMesh(mostRecentTop, "pyramid_top", "pyramid_top");
      state.addNetworking(mostRecentTop);
      state.addSceneNode(mostRecentTop, bottomId);

      state.createEntity(&mostRecentThrusters);
      state.addPlacement(mostRecentThrusters, ident);
      state.addMesh(mostRecentThrusters, "pyramid_thrusters", "pyramid_thrusters");
      state.addNetworking(mostRecentThrusters);
      state.addSceneNode(mostRecentThrusters, bottomId);

      state.createEntity(&mostRecentFire);
      state.addPlacement(mostRecentFire, pyrFirMat);
      state.addTransformFunction(mostRecentFire, getFireTransFuncDesc().registrationId);
      state.addMesh(mostRecentFire, "pyramid_thruster_flames", "pyramid_flames");
      state.addNetworking(mostRecentFire);
      state.addSceneNode(mostRecentFire, bottomId);

      entityId camGimbalId;
      state.createEntity(&camGimbalId);
      state.addPlacement(camGimbalId, ident);
      Placement *placement;
      state.getPlacement(camGimbalId, &placement);
      placement->forceLocalRotationAndScale = true;
      state.addMouseControls(camGimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY, true);
      state.addNetworking(camGimbalId);
      state.addSceneNode(camGimbalId, bottomId);

      state.createEntity(&mostRecentlyCreated);
      float back = 5.f;
      float tilt = 0.35f;
      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
      state.addPlacement(mostRecentlyCreated, camMat);
      state.addCamera(mostRecentlyCreated, settings::graphics::fovy, 0.1f, 10000.f);
      state.addNetworking(mostRecentlyCreated);
      state.addSceneNode(mostRecentlyCreated, camGimbalId);

      // Add the physics and controls to the base of the pyramid
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

      return mostRecentlyCreated; // All the id's can be derived from camId through the SceneNode component's parentId.
    }
    void broadcastLatest(ezecs::State &state, EntityComponentSystemInterface &ecs) {
      SceneNode *sceneNode;
      state.getSceneNode(mostRecentlyCreated, &sceneNode);
      entityId gimbalId = sceneNode->parentId;
      state.getSceneNode(gimbalId, &sceneNode);
      entityId ctrlId = sceneNode->parentId;
      ecs.broadcastManualEntity(ctrlId);
      ecs.broadcastManualEntity(mostRecentTop);
      ecs.broadcastManualEntity(mostRecentThrusters);
      ecs.broadcastManualEntity(mostRecentFire);
      ecs.broadcastManualEntity(gimbalId);
      ecs.broadcastManualEntity(mostRecentlyCreated);
    }
    void switchTo(State &state, const entityId & id) {
      publish<entityId>("switch_to_camera", id);
      SceneNode * sceneNode;
      state.getSceneNode(id, &sceneNode);
      publish<entityId>("switch_to_mouse_controls", sceneNode->parentId);
      state.getSceneNode(sceneNode->parentId, &sceneNode);
      publish<entityId>("switch_to_pyramid_controls", sceneNode->parentId);
    }
  }
}
