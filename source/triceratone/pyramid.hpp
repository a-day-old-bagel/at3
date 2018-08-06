
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
      ezecs::entityId topId, thrustersId, fireId;
      std::vector<ezecs::entityId> spheres;
      TransformFunctionDescriptor topTransFuncDesc, fireTransFuncDesc;

      void addToScene();

    public:

      ezecs::entityId bottomId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      Pyramid(ezecs::State &state, glm::mat4 &transform);
      void resizeFire();
      ezecs::entityId spawnSphere(bool shoot = false);
      void dropSphere();
      void shootSphere();
      void makeActiveControl(void* nothing);

  };
}
