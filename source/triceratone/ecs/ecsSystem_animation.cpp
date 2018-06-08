
#include <SDL_timer.h>
#include "ecsSystem_animation.hpp"

namespace at3 {

  AnimationSystem::AnimationSystem(State *state) : System(state) {
    name = "Animation System";
  }
  bool AnimationSystem::onInit() {
    return true;
  }
  void AnimationSystem::onTick(float dt) {
    for (auto id : registries[0].ids) {
      Placement* placement;
      state->get_Placement(id, &placement);
      TransformFunction* transformFunction;
      state->get_TransformFunction(id, &transformFunction);
      transformFunction->transformed = transformFunction->func(placement->mat, SDL_GetTicks());
    }
  }
}