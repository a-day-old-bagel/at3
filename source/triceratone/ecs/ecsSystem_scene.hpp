
#pragma once

#include "ezecs.hpp"
#include "vkc.hpp"
#include "ecsInterface.hpp"

using namespace ezecs;

namespace at3 {

  struct TransformFunctionDescriptor {
    uint8_t registrationId = 0;
    transformFunc func;
  };

  class SceneSystem : public System<SceneSystem> {
      SceneTree<EntityComponentSystemInterface> scene;
      std::shared_ptr<vkc::VulkanContext<EntityComponentSystemInterface>> vulkan;
      rtu::topics::Subscription setEcsInterfaceSub;
      rtu::topics::Subscription setVulkanContextSub;
      void setEcsInterface(void *ecsInterface);
      void setVulkanContext(void *vkc);

      std::vector<transformFunc> transformFuncs;
      rtu::topics::Subscription registerTransformFuncSub;
      void registerTransFunc(void *TransFuncDesc);

    public:
      std::vector<compMask> requiredComponents = {
          SCENENODE,
          TRANSFORMFUNCTION,
          MESH
      };
      SceneSystem(State* state);
      virtual ~SceneSystem();
      bool onInit();
      void onTick(float dt);
      bool onDiscoverSceneNode(const entityId &id);
      bool onForgetSceneNode(const entityId &id);
      bool onDiscoverMesh(const entityId &id);
      bool onForgetMesh(const entityId &id);
  };
}
