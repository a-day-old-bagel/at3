#pragma once

#include "interface.hpp"

namespace at3 {
  namespace PlayerSet {
    ezecs::entityId create(ezecs::State &state, EntityComponentSystemInterface &ecs, uint32_t role);
    void fill(ezecs::State &state, const ezecs::entityId & id, EntityComponentSystemInterface &ecs,
        const glm::mat4 &mat);
  }
}
