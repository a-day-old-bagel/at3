
#pragma once

#include "ezecs.hpp"
#include "vkc.hpp"
#include "interface.hpp"

using namespace ezecs;

namespace at3 {
  class SceneSystem : public System<SceneSystem> {

      SceneTree<EntityComponentSystemInterface> scene;
      std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>> vulkan;
      std::shared_ptr<EntityComponentSystemInterface> ecs;
      std::vector<transformFunc> transformFuncs;

      void setEcsInterface(void *ecsInterface);
      void setVulkanContext(void *vkc);
      void registerTransFunc(void *TransFuncDesc);

    public:
      std::vector<compMask> requiredComponents = {
          SCENENODE,
          TRANSFORMFUNCTION,
          MESH,
          PLAYER
      };
      explicit SceneSystem(State* state);
      ~SceneSystem() override;
      bool onInit();
      void onTick(float dt);
      bool onDiscoverSceneNode(const entityId &id);
      bool onForgetSceneNode(const entityId &id);
      bool onDiscoverMesh(const entityId &id);
      bool onForgetMesh(const entityId &id);
      bool onDiscoverPlayer(const entityId &id);
      bool onForgetPlayer(const entityId &id);
  };
}
