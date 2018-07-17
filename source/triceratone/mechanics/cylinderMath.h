
#pragma once

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#if USE_VULKAN_COORDS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <btBulletDynamicsCommon.h> // for bullet vector types

namespace at3 {

  glm::vec3 bulletToGlm(const btVector3& vec);
  btVector3 glmToBullet(const glm::vec3& vec);

  glm::vec3 getCylGrav(const glm::vec3 & pos, const glm::vec3 & nativeVel);
  glm::vec3 getNaiveCylGrav(const glm::vec3 &pos);
  glm::vec3 getNaiveCylGravDir(const glm::vec3 &pos);
  glm::mat3 getCylStandingRot(const glm::vec3 &pos, const float &pitch, const float &yaw);
}
