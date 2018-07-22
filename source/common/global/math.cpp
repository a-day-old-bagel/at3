
#include <SDL.h>
#include "math.hpp"

namespace at3 {
  const float pi = (float)M_PI;
  const float halfPi = pi * 0.5f;
  const float twoPi = pi * 2.f;
  const float rpm = twoPi / 60.f; // multiply by this to go from revolutions/minute to radians/sec.
  const float msToS = 0.001f; // multiply by this to go from revolutions/minute to radians/sec.

  glm::vec3 bulletToGlm(const btVector3 &vec) {
    return {vec.x(), vec.y(), vec.z()};
  }
  btVector3 glmToBullet(const glm::vec3& vec) {
    return {vec.x, vec.y, vec.z};
  }
  glm::quat bulletToGlm(const btQuaternion& vec) {
    return glm::quat(vec.x(), vec.y(), vec.z(), vec.w());
  }
  btQuaternion glmToBullet(const glm::quat& vec) {
    return {vec.x, vec.y, vec.z, vec.w};
  }
}
