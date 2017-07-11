
#pragma once

#include "dualityInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class BasicWalker {
    private:
      ezecs::State* mpState;
      Scene_* mpScene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> mpBody;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;

      BasicWalker(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
  };
}
