
#include <SDL_timer.h>
#include "scene.hpp"
#include "serialization.hpp"

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
    ecs = *(std::shared_ptr<EntityComponentSystemInterface>*) ecsInterface;
    Object::linkEcs(ecs);
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
    if (sceneNode->parentId) {  // ID 0 indicates no parent
      scene.addChildObject(sceneNode->parentId, id);
    } else {
      scene.addObject(id);
    }
    return true;
  }
  bool SceneSystem::onForgetSceneNode(const entityId &id) {
    SceneNode *sceneNode;
    state->getSceneNode(id, &sceneNode);
    // TODO: maybe get the node's children and reassign their parent field to this object's parent here instead of
    // inside SceneTree
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
    Player *player;
    state->getPlayer(id, &player);

    glm::mat4 ident(1.f);

    // FIXME: until the local-end temporary IDS are set up (if ever), clients MUST NOT create any entities here.
    // currently, that won't happen under normal circumstances since a server creates all the control entities before a
    // client can reach this code.

    // TODO: if player->free is nonzero, should also check if that ID has the correct components? Or just let it crash?
    if ( ! player->free) {
//      float back = 0.f;
//      float tilt = 0.f;
//      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt, glm::vec3(1.0f, 0.0f, 0.0f));
//      ecs->openEntityRequest();
//      ecs->requestPlacement(camMat);
//      ecs->requestCamera(settings::graphics::fovy, 0.1f, 10000.f);
//      entityId camId = ecs->closeEntityRequest();
//
//      ecs->openEntityRequest();
//      ecs->requestPlacement(ident);
//      ecs->requestMouseControls(settings::controls::mouseInvertX, settings::controls::mouseInvertY);
//      entityId gimbalId = ecs->closeEntityRequest();
//
//      Placement *placement;
//      state->getPlacement(gimbalId, &placement);
//      placement->forceLocalRotationAndScale = true;
//
//      glm::mat4 start = glm::translate(ident, {0, -790, -120});
//      ecs->openEntityRequest();
//      ecs->requestPlacement(start);
//      ecs->requestFreeControls(gimbalId);
//      ecs->closeEntityRequest();



      entityId camId, gimbalId, ctrlId;

      state->createEntity(&camId);
      float back = 0.f;
      float tilt = 0.f;
      glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
      state->addPlacement(camId, camMat);
      state->addCamera(camId, settings::graphics::fovy, 0.1f, 10000.f);

      state->createEntity(&gimbalId);
      state->addPlacement(gimbalId, ident);
      Placement *placement;
      state->getPlacement(gimbalId, &placement);
      placement->forceLocalRotationAndScale = true;
      state->addMouseControls(
          gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);

      // TODO: make the spawn location something less hard-coded (probably an asset thing ... ugh)
      glm::mat4 start = glm::translate(ident, {0, -790, -120});
      state->createEntity(&ctrlId);
      state->addPlacement(ctrlId, start);
      state->addFreeControls(ctrlId, gimbalId);

      state->addSceneNode(ctrlId, 0);
      state->addSceneNode(gimbalId, ctrlId);
      state->addSceneNode(camId, gimbalId);

      player->free = camId;

      ecs->broadcastManualEntity(ctrlId);
      ecs->broadcastManualEntity(gimbalId);
      ecs->broadcastManualEntity(camId);
    }

    if ( ! player->walk) {

    }

    if ( ! player->pyramid) {

    }

    if ( ! player->track) {

    }

    ecs->broadcastManualEntity(id);


//    // the free cameras
//    state.createEntity(&players.back().camId);
//    float back = 0.f;
//    float tilt = 0.f;
//    glm::mat4 camMat = glm::rotate(glm::translate(ident, {0.f, 0.f, back}), tilt , glm::vec3(1.0f, 0.0f, 0.0f));
//    state.addPlacement(players.back().camId, camMat);
//    state.addCamera(players.back().camId, settings::graphics::fovy, 0.1f, 10000.f);
//
//    state.createEntity(&players.back().gimbalId);
//    state.addPlacement(players.back().gimbalId, ident);
//    Placement *placement;
//    state.getPlacement(players.back().gimbalId, &placement);
//    placement->forceLocalRotationAndScale = true;
//    state.addMouseControls(
//        players.back().gimbalId, settings::controls::mouseInvertX, settings::controls::mouseInvertY);
//
//    glm::mat4 start = glm::translate(ident, {0, -790, -120});
//    state.createEntity(&players.back().ctrlId);
//    state.addPlacement(players.back().ctrlId, start);
//    state.addFreeControls(players.back().ctrlId, players.back().gimbalId);
//
//    state.addSceneNode(players.back().ctrlId, 0);
//    state.addSceneNode(players.back().gimbalId, players.back().ctrlId);
//    state.addSceneNode(players.back().camId, players.back().gimbalId);


    return true;
  }

  bool SceneSystem::onForgetPlayer(const entityId &id) {
    // TODO: delete player avatars
    return true;
  }
}
