#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"

namespace at3 {
  class DuneBuggy {
    private:
      std::vector<std::shared_ptr<Mesh>> wheels;
      ezecs::State* state;
      Scene* scene;
      void addToScene();
    public:
      std::shared_ptr<Mesh> chassis;
      std::shared_ptr<ThirdPersonCamera> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context,
                  Scene &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
