
#include <SDL_timer.h>
#include "ecsSystem_scene.hpp"
#include "vkc.hpp"
#include "ecsInterface.hpp"

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
    registries[1].discoverHandler = RTU_MTHD_DLGT(&SceneSystem::onDiscoverMesh, this);
    registries[1].forgetHandler = RTU_MTHD_DLGT(&SceneSystem::onForgetMesh, this);
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
  bool SceneSystem::onDiscoverMesh(const entityId &id) {
    Mesh *mesh;
    state->get_Mesh(id, &mesh);

    vkc::MeshRegistrationInfo<EntityComponentSystemInterface> info;
    info.id = id;
    info.meshFileName = mesh->meshFileName;
    info.textureFileName = mesh->textureFileName;

    rtu::topics::publish<vkc::MeshRegistrationInfo<EntityComponentSystemInterface>>("mesh_registration", info);

    return true;
  }
  bool SceneSystem::onForgetMesh(const entityId &id) {
    vkc::MeshRegistrationInfo<EntityComponentSystemInterface> info;
    info.deRegister = true;
    info.id = id;
    rtu::topics::publish<vkc::MeshRegistrationInfo<EntityComponentSystemInterface>>("mesh_registration", info);
    return true;
  }
}
