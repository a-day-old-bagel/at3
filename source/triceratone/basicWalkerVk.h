
#pragma once

#include "ecsInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class BasicWalkerVk {
    private:
      ezecs::State* mpState;
      Scene_* mpScene;
      void addToScene();
      glm::mat4 bodyVisualTransform(const glm::mat4 &transformIn, uint32_t time);
    public:
      std::shared_ptr<SceneObject_> mpPhysicsBody;
      std::shared_ptr<MeshObjectVk_> mpVisualBody;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      BasicWalkerVk(ezecs::State &state, VulkanContext<EntityComponentSystemInterface> *context, Scene_ &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
