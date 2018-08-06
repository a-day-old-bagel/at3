#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"
#include "ecsSystem_scene.hpp"

namespace at3 {
  class DuneBuggy {
    private:

      ezecs::State* state;
      std::vector<ezecs::entityId> wheels;
      TransformFunctionDescriptor wheelTransFuncDesc;

      void addToScene();

    public:

      ezecs::entityId chassisId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      DuneBuggy(ezecs::State &state, glm::mat4 &transform);

      void makeActiveControl(void* nothing);

  };
}
