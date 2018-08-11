
#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"
#include "ecsSystem_scene.hpp"

namespace at3 {
  class Pyramid {
    private:

      ezecs::State* state;
      std::shared_ptr<EntityComponentSystemInterface> ecs;

      ezecs::entityId topId, thrustersId, fireId;
      std::vector<ezecs::entityId> spheres;

      void addToScene();

    public:

      ezecs::entityId bottomId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      Pyramid(ezecs::State &state, std::shared_ptr<EntityComponentSystemInterface> ecs, glm::mat4 &transform);
      ezecs::entityId spawnSphere(bool shoot = false);
      void dropSphere();
      void shootSphere();
      void makeActiveControl(void* nothing);
      const static TransformFunctionDescriptor & getTopTransFuncDesc();
      const static TransformFunctionDescriptor & getFireTransFuncDesc();

  };
}
