
#include <SDL_timer.h>
#include "scene.hpp"
#include "serialization.hpp"
#include "playerSet.hpp"

namespace at3 {

  SceneSystem::SceneSystem(State *state) : System(state) {
    name = "Animation System";
    // Static subscriptions will only apply to the first instance of this class created. But usually only one exists.
    RTU_STATIC_SUB(setEcsInterfaceSub, "set_ecs_interface", SceneSystem::setEcsInterface, this);
    RTU_STATIC_SUB(setVulkanContextSub, "set_vulkan_context", SceneSystem::setVulkanContext, this);
    RTU_STATIC_SUB(registerTransformFuncSub, "register_transform_function", SceneSystem::registerTransFunc, this);
  }
  SceneSystem::~SceneSystem() {
    SceneObject<EntityComponentSystemInterface>::resetEcs();
  }
  void SceneSystem::setEcsInterface(void *ecsInterface) {
    ecs = *(std::shared_ptr<EntityComponentSystemInterface>*) ecsInterface;
    SceneObject<EntityComponentSystemInterface>::linkEcs(ecs);
  }
  void SceneSystem::setVulkanContext(void *vkc) {
    vulkan = *(std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>>*) vkc;
  }
  void SceneSystem::registerTransFunc(void *TransFuncDesc) {
    auto desc = (TransformFunctionDescriptor*)TransFuncDesc;
    transformFuncs.push_back(desc->func);
    desc->registrationId = transformFuncs.size(); // starts at 1 so that 0 can be used to indicate non-initialization.
  }
  bool SceneSystem::onInit() {
    registries[0].discoverHandler = RTU_MTHD_DLGT(&SceneSystem::onDiscoverSceneNode, this);
    registries[0].forgetHandler = RTU_MTHD_DLGT(&SceneSystem::onForgetSceneNode, this);
    registries[2].discoverHandler = RTU_MTHD_DLGT(&SceneSystem::onDiscoverMesh, this);
    registries[2].forgetHandler = RTU_MTHD_DLGT(&SceneSystem::onForgetMesh, this);
    registries[3].discoverHandler = RTU_MTHD_DLGT(&SceneSystem::onDiscoverPlayer, this);
    registries[3].forgetHandler = RTU_MTHD_DLGT(&SceneSystem::onForgetPlayer, this);
    return true;
  }
  void SceneSystem::onTick(float dt) {
    uint32_t currentTime = SDL_GetTicks();
    TransFuncEcsContext ctxt = {};
    ctxt.ecs = (void*)state;
    for (auto id : registries[1].ids) {
      Placement* placement;
      state->getPlacement(id, &placement);
      TransformFunction* transformFunction;
      state->getTransformFunction(id, &transformFunction);
      ctxt.id = id;
      transformFunction->transformed = transformFuncs[transformFunction->transFuncId - 1] // indexed from 1 - shift to 0
          (placement->mat, placement->absMat, currentTime, &ctxt);
    }
    scene.updateAbsoluteTransformCaches();
  }
  bool SceneSystem::onDiscoverSceneNode(const entityId &id) {
    SceneNode *sceneNode;
    state->getSceneNode(id, &sceneNode);
    scene.addObject(id, sceneNode->parentId);
    return true;
  }
  bool SceneSystem::onForgetSceneNode(const entityId &id) {
    SceneNode *sceneNode;
    state->getSceneNode(id, &sceneNode);
    scene.removeObject(id, sceneNode->parentId);
    return true;
  }
  bool SceneSystem::onDiscoverMesh(const entityId &id) {
    Mesh *mesh;
    state->getMesh(id, &mesh);
    vulkan->registerMeshInstance(id, mesh->meshFileName, mesh->textureFileName);
    return true;
  }
  bool SceneSystem::onForgetMesh(const entityId &id) {
    vulkan->deRegisterMeshInstance(id);
    return true;
  }

  bool SceneSystem::onDiscoverPlayer(const entityId &id) {
    // TODO: Don't have this position hard-coded here.
    glm::mat4 ident(1.f);
    glm::mat4 start = glm::translate(ident, {0, -790, -120});
    PlayerSet::fill(*state, id, *ecs, start);
    return true;
  }

  bool SceneSystem::onForgetPlayer(const entityId &id) {
    // TODO: delete player avatars
    return true;
  }
}
