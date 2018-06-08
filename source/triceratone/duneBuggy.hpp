#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "scene.hpp"
#include "meshObject.hpp"

namespace at3 {
  class DuneBuggy {
    private:
      std::vector<std::shared_ptr<MeshObject_>> wheels;
      ezecs::State* state;
      Scene_* scene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> chassis;
      std::shared_ptr<ThirdPersonCamera_> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void tip();
      void makeActiveControl(void* nothing);
  };
}
