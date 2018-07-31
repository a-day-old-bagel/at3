
#pragma once

#include "ezecs.hpp"
#include "ecsInterface.hpp"

using namespace ezecs;

namespace at3 {
  class SceneSystem : public System<SceneSystem> {
      SceneTree<EntityComponentSystemInterface> scene;
      rtu::topics::Subscription setEcsInterfaceSub;
      void setEcsInterface(void *ecsInterface);
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
  };
}
