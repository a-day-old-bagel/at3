
#include <SDL_timer.h>
#include "ecsSystem_scene.hpp"

namespace at3 {

  SceneSystem::SceneSystem(State *state)
      : System(state),
        setEcsInterfaceSub("set_ecs_interface", RTU_MTHD_DLGT(&SceneSystem::setEcsInterface, this))
  {
    name = "Animation System";
  }
  SceneSystem::~SceneSystem() {
    Object::resetEcs();
  }
  bool SceneSystem::onInit() {
    registries[0].discoverHandler = RTU_MTHD_DLGT(&SceneSystem::onDiscoverSceneNode, this);
    registries[0].forgetHandler = RTU_MTHD_DLGT(&SceneSystem::onForgetSceneNode, this);
    return true;
  }
  void SceneSystem::onTick(float dt) {
    for (auto id : registries[1].ids) {
      Placement* placement;
      state->get_Placement(id, &placement);
      TransformFunction* transformFunction;
      state->get_TransformFunction(id, &transformFunction);
      transformFunction->transformed = transformFunction->func(placement->mat, placement->absMat, SDL_GetTicks());
    }
    scene.updateAbsoluteTransformCaches();
  }
  bool SceneSystem::onDiscoverSceneNode(const entityId &id) {
    SceneNode *sceneNode;
    state->get_SceneNode(id, &sceneNode);
    if (sceneNode->parentId) {  // ID 0 indicates no parent
      scene.addChildObject(sceneNode->parentId, id);
    } else {
      scene.addObject(id);
    }
    return true;
  }
  bool SceneSystem::onForgetSceneNode(const entityId &id) {
    SceneNode *sceneNode;
    state->get_SceneNode(id, &sceneNode);
    // TODO: maybe get the node's children and reassign their parent field to this object's parent here instead of
    // inside SceneTree
    return true;
  }
  void SceneSystem::setEcsInterface(void *ecsInterface) {
    Object::linkEcs( *(std::shared_ptr<EntityComponentSystemInterface>*) ecsInterface );
  }
}
