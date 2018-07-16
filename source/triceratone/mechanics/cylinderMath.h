
#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace at3 {

  glm::vec3 getCylGrav(const glm::vec3 & pos);
  glm::vec3 getCylGravDir(const glm::vec3 & pos);
  glm::mat3 getCylStandingRot(const glm::vec3 & pos);
  glm::mat3 getCylStandingRot(const float &pitch, const float &yaw);
  glm::mat3 getCylStandingRot(const glm::vec3 &pos, const float &pitch, const float &yaw);
}
