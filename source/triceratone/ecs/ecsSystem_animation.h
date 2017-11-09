
#ifndef ECSSYSTEM_MOVEMENT_H
#define ECSSYSTEM_MOVEMENT_H

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

#endif //ECSSYSTEM_MOVEMENT_H
