
#pragma once

#include "ezecs.hpp"

using namespace ezecs;

namespace at3 {
  class AnimationSystem : public System<AnimationSystem> {
    public:
      std::vector<compMask> requiredComponents = {
          TRANSFORMFUNCTION
      };
      AnimationSystem(State* state);
      bool onInit();
      void onTick(float dt);
  };
}
