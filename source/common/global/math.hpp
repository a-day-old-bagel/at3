#pragma once

#include "definitions.hpp"

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <btBulletDynamicsCommon.h>

namespace at3 {
  extern const float pi;
  extern const float halfPi;
  extern const float twoPi;
  extern const float rpm; // multiply by this to go from revolutions/minute to radians/sec.
  extern const float msToS; // multiply by this to go from milliseconds to seconds

  template <typename T> int sign(T val) {
    return (T(0) < val) - (val < T(0));
  }

  glm::vec3 bulletToGlm(const btVector3& vec);
  btVector3 glmToBullet(const glm::vec3& vec);
//  glm::quat bulletToGlm(const btQuaternion& vec); // FIXME: NOT WORKING
//  btQuaternion glmToBullet(const glm::quat& vec); // FIXME: NOT WORKING
}
