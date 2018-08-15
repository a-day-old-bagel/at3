
#include "duneBuggy.hpp"
#include "serialization.hpp"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {

  namespace DuneBuggy {

    static glm::mat4 wheelScaler(
        const glm::mat4& transIn,
        const glm::mat4& absTransIn,
        uint32_t time,
        TransFuncEcsContext *ctxt = nullptr) {
      return glm::scale(transIn, glm::vec3(WHEEL_RADIUS * 2.f));
    }
    TransformFunctionDescriptor & getWheelTransFuncDesc() {
      static TransformFunctionDescriptor wheelTransFuncDesc;
      if ( ! wheelTransFuncDesc.registrationId) {
        wheelTransFuncDesc.func = RTU_FUNC_DLGT(wheelScaler);
        rtu::topics::publish<TransformFunctionDescriptor>("register_transform_function", wheelTransFuncDesc);
      }
      return wheelTransFuncDesc;
    }

    entityId create(State &state, const glm::mat4 &transform) {
      // The order of entity creation matters here, unfortunately, because it determines the order of the id's in
      // the Network System's registry, which determines the order in which the entities are sent to each new client.
      // And if the clients receive those in the wrong order, they can't build their scene trees correctly.

      glm::mat4 ident(1.f);

      entityId chassisId;
      state.createEntity(&chassisId);
      state.addPlacement(chassisId, transform);
      state.addMesh(chassisId, "chassis", "cliff1024_00");
      std::shared_ptr<void> chassisVerts = std::make_shared<std::vector<float>>(std::vector<float> {
          1.7f,  1.7f, -0.4f,
          1.7f, -1.7f, -0.4f,
          -1.7f, -1.7f, -0.4f,
          -1.7f,  1.7f, -0.4f,
          2.1f,  2.1f,  0.4f,
          2.1f, -2.1f,  0.4f,
          -2.1f, -2.1f,  0.4f,
          -2.1f,  2.1f,  0.4f
      });
      // Chassis physics object
      state.addPhysics(chassisId, 50.f, chassisVerts, Physics::DYNAMIC_CONVEX_MESH);
      Physics *physics;
      state.getPhysics(chassisId, &physics);
      physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
      // The slow stuff (CCD)
      physics->rigidBody->setCcdMotionThreshold(1);
      physics->rigidBody->setCcdSweptSphereRadius(0.2f);
      // Tank-like movement controls
      state.addTrackControls(chassisId);
      TrackControls *trackControls;
      state.getTrackControls(chassisId, &trackControls);
      trackControls->tuning.m_suspensionStiffness = 16.f;     // 5.88f
      trackControls->tuning.m_suspensionCompression = 0.8f;   // 0.83f
      trackControls->tuning.m_suspensionDamping = 1.0f;       // 0.88f
      trackControls->tuning.m_maxSuspensionTravelCm = 80.f;   // 500.f
      trackControls->tuning.m_frictionSlip  = 40.f;           // 10.5f
      trackControls->tuning.m_maxSuspensionForce  = 20000.f;  // 6000.f
      state.addNetworking(chassisId);
      state.addSceneNode(chassisId, 0);

      btVector3 wheelConnectionPoints[4] {
          {-1.9f,  1.9f, 0.f},
          { 1.9f,  1.9f, 0.f},
          {-1.9f, -1.9f, 0.f},
          { 1.9f, -1.9f, 0.f}
      };
      for (const auto & wheelConnectionPoint : wheelConnectionPoints) {
        entityId wheelId;
        state.createEntity(&wheelId);
        state.addPlacement(wheelId, ident);
        state.addMesh(wheelId, "wheel", "tire");
        std::shared_ptr<WheelInitInfo> wheelInitInfo = std::make_shared<WheelInitInfo>( WheelInitInfo {
            {                         // WheelInfo struct - this part of the wheelInitInfo will persist.
                chassisId,                    // id of wheel's parent entity (chassis)
                wheelId,                      // wheel's own id (used for removal upon chassis deletion)
                -1,                           // bullet's index for this wheel (will be set correctly when wheel is added)
                wheelConnectionPoint.x()  // x component of wheel's chassis-space connection point
            },
            wheelConnectionPoint, // connection point
            {0.f, 0.f, -1.f},         // direction
            {1.f, 0.f, 0.f},          // axle
            0.4f,                     // suspension rest length
            WHEEL_RADIUS,             // wheel radius
            false                     // is front wheel
        });
        state.addPhysics(wheelId, 10.f, wheelInitInfo, Physics::WHEEL);
        state.addTransformFunction(wheelId, getWheelTransFuncDesc().registrationId);
        state.addNetworking(wheelId);
        state.addSceneNode(wheelId, 0);
      }

      entityId camGimbalId;
      state.createEntity(&camGimbalId);
      state.addPlacement(camGimbalId, ident);
      Placement *placement;
      state.getPlacement(camGimbalId, &placement);
      placement->forceLocalRotationAndScale = true;
      state.addMouseControls(camGimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);
      state.addNetworking(camGimbalId);
      state.addSceneNode(camGimbalId, chassisId);

      entityId camId;
      state.createEntity(&camId);
      float back = 10.f;
      float tilt = 0.35f;
      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
      state.addPlacement(camId, camMat);
      state.addCamera(camId, settings::graphics::fovy, 0.1f, 10000.f);
      state.addNetworking(camId);
      state.addSceneNode(camId, camGimbalId);

      return camId; // All these id's can be derived from camId through the SceneNode component's parentId.
    }
    void broadcast(State &state, EntityComponentSystemInterface &ecs, const entityId & id) {
      SceneNode *sceneNode;
      state.getSceneNode(id, &sceneNode);
      entityId gimbalId = sceneNode->parentId;
      state.getSceneNode(gimbalId, &sceneNode);
      entityId ctrlId = sceneNode->parentId;
      ecs.broadcastManualEntity(ctrlId);
      TrackControls *trackControls;
      state.getTrackControls(ctrlId, &trackControls);
      for (const auto & wheel : trackControls->wheels) {
        ecs.broadcastManualEntity(wheel.myId);
      }
      ecs.broadcastManualEntity(gimbalId);
      ecs.broadcastManualEntity(id);
    }
    void switchTo(State &state, const entityId & id) {
      publish<entityId>("switch_to_camera", id);
      SceneNode * sceneNode;
      state.getSceneNode(id, &sceneNode);
      publish<entityId>("switch_to_mouse_controls", sceneNode->parentId);
      state.getSceneNode(sceneNode->parentId, &sceneNode);
      publish<entityId>("switch_to_track_controls", sceneNode->parentId);
    }
    void destroy(State &state, const entityId & id) {

    }
  }
}
