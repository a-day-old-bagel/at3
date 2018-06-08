//
// Created by volundr on 4/2/17.
//

#include "bulletDebug.hpp"
namespace at3 {
  /*
   * Define conversion from Bullet vector structures to glm vector structures
   */
  glm::vec3 bulletToGlm(const btVector3 &vec) {
    return {vec.x(), vec.y(), vec.z()};
  }

  btVector3 glmToBullet(const glm::vec3& vec) {
    return btVector3(vec.x, vec.y, vec.z);
  }
}