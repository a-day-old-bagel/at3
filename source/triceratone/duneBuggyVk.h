#pragma once

#include <vector>
#include "ecsInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class DuneBuggyVk {
    private:
      std::vector<std::shared_ptr<MeshObjectVk_>> mvpWheels;
      ezecs::State* mpState;
      Scene_* mpScene;
      void addToScene();
    public:
      std::shared_ptr<MeshObjectVk_> mpChassis;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      DuneBuggyVk(ezecs::State &state, VulkanContext<EntityComponentSystemInterface> *context, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void tip();
      void makeActiveControl(void* nothing);
  };
}
