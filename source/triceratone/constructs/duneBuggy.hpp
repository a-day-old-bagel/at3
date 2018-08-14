#pragma once

#include <vector>
#include "interface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"

namespace at3 {
  class DuneBuggy {
    private:

      ezecs::State* state;
      std::vector<ezecs::entityId> wheels;

      void addToScene();

    public:

      ezecs::entityId chassisId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, glm::mat4 &transform);

      void makeActiveControl(void* nothing);
      const static ezecs::TransformFunctionDescriptor & getWheelTransFuncDesc();
  };
}
