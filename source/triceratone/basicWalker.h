
#pragma once

#include "ecsInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class BasicWalker {
    private:
      ezecs::State* state;
      Scene_* scene;
      void addToScene();
      glm::mat4 bodyVisualTransform(const glm::mat4 &transformIn, uint32_t time);
    public:
      std::shared_ptr<SceneObject_> physicsBody;
      std::shared_ptr<MeshObject_> visualBody;
      std::shared_ptr<ThirdPersonCamera_> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      BasicWalker(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
