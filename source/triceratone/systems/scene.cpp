
#include <SDL_timer.h>
#include "scene.hpp"

namespace at3 {

  SceneSystem::SceneSystem(State *state)
      : System(state),
        setEcsInterfaceSub("set_ecs_interface", RTU_MTHD_DLGT(&SceneSystem::setEcsInterface, this)),
        setVulkanContextSub("set_vulkan_context", RTU_MTHD_DLGT(&SceneSystem::setVulkanContext, this)),
        registerTransformFuncSub("register_transform_function", RTU_MTHD_DLGT(&SceneSystem::registerTransFunc, this))
  {
    name = "Animation System";
  }
  SceneSystem::~SceneSystem() {
    Object::resetEcs();
  }
  void SceneSystem::setEcsInterface(void *ecsInterface) {
    Object::linkEcs( *(std::shared_ptr<EntityComponentSystemInterface>*) ecsInterface );
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
    return true;
  }
  void SceneSystem::onTick(float dt) {
    uint32_t currentTime = SDL_GetTicks();
    TransFuncEcsContext ctxt = {};
    ctxt.ecs = (void*)state;
    for (auto id : registries[1].ids) {
      Placement* placement;
      state->get_Placement(id, &placement);
      TransformFunction* transformFunction;
      state->get_TransformFunction(id, &transformFunction);
      ctxt.id = id;
      transformFunction->transformed = transformFuncs[transformFunction->transFuncId - 1] // indexed from 1 - shift to 0
          (placement->mat, placement->absMat, currentTime, &ctxt);
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
  bool SceneSystem::onDiscoverMesh(const entityId &id) {
    Mesh *mesh;
    state->get_Mesh(id, &mesh);
    vulkan->registerMeshInstance(id, mesh->meshFileName, mesh->textureFileName);
    return true;
  }
  bool SceneSystem::onForgetMesh(const entityId &id) {
    vulkan->deRegisterMeshInstance(id);
    return true;
  }
}
