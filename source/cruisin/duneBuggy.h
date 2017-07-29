#pragma once

#include <vector>
#include "dualityInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class DuneBuggy {
    private:
      std::vector<std::shared_ptr<MeshObject_>> mvpWheels;
      ezecs::State* mpState;
      Scene_* mpScene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> mpChassis;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void tip();
  };
}
