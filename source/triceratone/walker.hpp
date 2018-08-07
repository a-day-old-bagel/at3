
#pragma once

#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"
#include "ecsSystem_scene.hpp"

namespace at3 {
  class Walker {
    private:

      ezecs::State* state;
      ezecs::entityId visualId;

      void addToScene();

    public:

      ezecs::entityId physicalId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      Walker(ezecs::State &state, glm::mat4 &transform);

      void makeActiveControl(void* nothing);
      const static TransformFunctionDescriptor & getBodyTransFuncDesc();
  };
}
