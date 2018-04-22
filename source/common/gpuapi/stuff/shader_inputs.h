#pragma once

#include "config.h"

#if USE_AT3_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <glm/glm.hpp>

//for UBO / SSBO testing, we'll use transform data
struct VShaderInput {
  glm::mat4 model;
  glm::mat4 normal;
};
