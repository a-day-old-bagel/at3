
#pragma once

#include "interface.hpp"

namespace at3 {
  namespace Walker {
    ezecs::entityId create(ezecs::State &state, const glm::mat4 &transform, const ezecs::entityId &id = 0);
    void broadcastLatest(ezecs::State &state, EntityComponentSystemInterface &ecs);
    void switchTo(ezecs::State &state, const ezecs::entityId & id);
    const ezecs::TransformFunctionDescriptor & getBodyTransFuncDesc();
  }
}
