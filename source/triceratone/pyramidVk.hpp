//
// Created by volundr on 4/27/17.
//

#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "scene.hpp"
#include "vkc.hpp"

namespace at3 {
  class PyramidVk {
    private:
      std::shared_ptr<MeshObjectVk_> top, thrusters, fire;
      std::vector<std::shared_ptr<MeshObjectVk_>> spheres;
      ezecs::State* state;
      Scene_* scene;
      vkc::VulkanContext<EntityComponentSystemInterface>* vkc;
      void addToScene();
    public:
      std::shared_ptr<MeshObjectVk_> base;
      std::shared_ptr<ThirdPersonCamera_> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      PyramidVk(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context, Scene_ &scene,
                glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
