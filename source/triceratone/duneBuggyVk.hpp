#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "scene.hpp"
#include "meshObject.hpp"

namespace at3 {
  class DuneBuggyVk {
    private:
      std::vector<std::shared_ptr<MeshObjectVk_>> wheels;
      ezecs::State* state;
      Scene_* scene;
      void addToScene();
    public:
      std::shared_ptr<MeshObjectVk_> chassis;
      std::shared_ptr<ThirdPersonCamera_> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      DuneBuggyVk(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context,
                  Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void tip();
      void makeActiveControl(void* nothing);
  };
}
