//
// Created by volundr on 4/27/17.
//

#pragma once

#include <vector>
#include "ecsInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "vulkanContext.h"

namespace at3 {
  class PyramidVk {
    private:
      std::shared_ptr<MeshObjectVk_> mpTop, mpThrusters, mpFire;
      std::vector<std::shared_ptr<MeshObjectVk_>> mvpSpheres;
      ezecs::State* mpState;
      Scene_* mpScene;
      VulkanContext<EntityComponentSystemInterface>* vkc;
      void addToScene();
    public:
      std::shared_ptr<MeshObjectVk_> mpBase;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      PyramidVk(ezecs::State &state, VulkanContext<EntityComponentSystemInterface> *context, Scene_ &scene,
                glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
