#pragma once

#include "configuration.h"

#if USE_AT3_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

struct VShaderInput {
  glm::mat4 model;
  glm::mat4 normal;
};
