
#pragma once

#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"

namespace at3 {
  class Walker {
    private:
      ezecs::State* state;
      Scene* scene;
      void addToScene();
      glm::mat4 bodyVisualTransform(const glm::mat4 &transIn, const glm::mat4& absTransIn, uint32_t time);
    public:
      std::shared_ptr<Object> physicsBody;
      std::shared_ptr<Mesh> visualBody;
      std::shared_ptr<ThirdPersonCamera> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      Walker(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context,
                    Scene &scene, glm::mat4 &transform);
      std::shared_ptr<PerspectiveCamera> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
