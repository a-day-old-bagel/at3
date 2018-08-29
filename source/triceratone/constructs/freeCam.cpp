
#include "freeCam.hpp"
#include "serialization.hpp"

using namespace ezecs;
using namespace rtu::topics;

namespace at3 {
  namespace FreeCam {

    static entityId mostRecentlyCreated = 0;

    entityId create(State &state, const glm::mat4 &transform, const entityId &id) {
      // The order of entity creation matters here, unfortunately, because it determines the order of the id's in
      // the Network System's registry, which determines the order in which the entities are sent to each new client.
      // And if the clients receive those in the wrong order, they can't build their scene trees correctly.

      glm::mat4 ident(1.f);

      // The base objects controlled by WASD
      entityId ctrlId;
      state.createEntity(&ctrlId);
      state.addPlacement(ctrlId, transform);
      state.addNetworking(ctrlId);
      state.addSceneNode(ctrlId, 0);

      // The camera gimbal controlled by the mouse
      entityId gimbalId;
      state.createEntity(&gimbalId);
      state.addPlacement(gimbalId, ident);
      Placement *placement;
      state.getPlacement(gimbalId, &placement);
      state.addMouseControls(
          gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY, false);
      state.addNetworking(gimbalId);
      state.addSceneNode(gimbalId, ctrlId);

      // The references that the freeControl keeps of it's associated mouseControl (what a mess)
      state.addFreeControls(ctrlId, gimbalId);

      // The camera itself
      state.createEntity(&mostRecentlyCreated);
      float back = 0.f;
      float tilt = 0.f;
      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
      state.addPlacement(mostRecentlyCreated, camMat);
      state.addCamera(mostRecentlyCreated, settings::graphics::fovy, 0.1f, 10000.f);
      state.addNetworking(mostRecentlyCreated);
      state.addSceneNode(mostRecentlyCreated, gimbalId);

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
    }
    void switchTo(State &state, const entityId & id) {
      publish<entityId>("switch_to_camera", id);
      SceneNode * sceneNode;
      state.getSceneNode(id, &sceneNode);
      publish<entityId>("switch_to_mouse_controls", sceneNode->parentId);
      state.getSceneNode(sceneNode->parentId, &sceneNode);
      publish<entityId>("switch_to_free_controls", sceneNode->parentId);
    }
  }
}
