//
// Created by volundr on 4/2/17.
//

#include "bulletDebug.hpp"
namespace at3 {
  /*
   * Define conversion from Bullet vector structures to glm vector structures
   */
  glm::vec3 btToGlm(const btVector3 &vec) {
    return {vec.x(), vec.y(), vec.z()};
  }

  btVector3 glmToBt(const glm::vec3 &vec) {
    return btVector3(vec.x, vec.y, vec.z);
  }
}
