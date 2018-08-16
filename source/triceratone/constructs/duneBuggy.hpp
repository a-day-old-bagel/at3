#pragma once

#include "interface.hpp"

namespace at3 {
  namespace DuneBuggy {
    ezecs::entityId create(ezecs::State &state, const glm::mat4 &transform);
    void broadcastLatest(ezecs::State &state, EntityComponentSystemInterface &ecs);
    void switchTo(ezecs::State &state, const ezecs::entityId & id);
    const ezecs::TransformFunctionDescriptor & getWheelTransFuncDesc();
  }
}
