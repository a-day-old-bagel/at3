
#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"

namespace at3 {
  class PyramidVk {
    private:
      std::shared_ptr<Mesh> top, thrusters, fire;
      std::vector<std::shared_ptr<Mesh>> spheres;
      ezecs::State* state;
      Scene* scene;
      vkc::VulkanContext<EntityComponentSystemInterface>* vkc;
      void addToScene();
    public:
      std::shared_ptr<Mesh> base;
      std::shared_ptr<ThirdPersonCamera> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      PyramidVk(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context, Scene &scene,
                glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
      std::shared_ptr<PerspectiveCamera> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
