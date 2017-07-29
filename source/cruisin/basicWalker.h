
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
      glm::mat4 bodyVisualTransform(const glm::mat4 &transformIn, uint32_t time);
    public:
      std::shared_ptr<SceneObject_> mpPhysicsBody;
      std::shared_ptr<MeshObject_> mpVisualBody;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      BasicWalker(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
  };
}
