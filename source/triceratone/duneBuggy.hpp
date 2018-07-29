#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"

namespace at3 {
  class DuneBuggy {
    private:

//      std::vector<std::shared_ptr<Mesh>> wheels;
      std::vector<std::shared_ptr<Object>> wheels;

      ezecs::State* state;
      Scene* scene;
      void addToScene();
    public:

//      std::shared_ptr<Mesh> chassis;
//      std::shared_ptr<ThirdPersonCamera> camera;
      std::shared_ptr<Object> chassis;
      std::shared_ptr<Object> camera;
      std::shared_ptr<Object> gimbal;

//      ezecs::entityId ctrlId;
      ezecs::entityId chassisId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context,
                  Scene &scene, glm::mat4 &transform);

//      std::shared_ptr<PerspectiveCamera> getCamPtr();

      void makeActiveControl(void* nothing);
  };
}
