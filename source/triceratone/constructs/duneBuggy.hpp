#pragma once

#include "interface.hpp"

namespace at3 {
  namespace DuneBuggy {
    ezecs::entityId create(ezecs::State &state, const glm::mat4 &transform);
    void broadcast(ezecs::State &state, EntityComponentSystemInterface &ecs, const ezecs::entityId & id);
    void switchTo(ezecs::State &state, const ezecs::entityId & id);
    void destroy(ezecs::State &state, const ezecs::entityId & id);
    ezecs::TransformFunctionDescriptor & getWheelTransFuncDesc();
  }
}
