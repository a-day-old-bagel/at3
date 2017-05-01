//
// Created by volundr on 4/27/17.
//

#ifndef AT3_PYRAMID_H
#define AT3_PYRAMID_H

#include <vector>
#include "dualityInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class Pyramid {
    private:
      ezecs::State* state;
      Scene_* scene;
      std::shared_ptr<MeshObject_> top, thrusters, fire;
      std::vector<std::shared_ptr<MeshObject_>> spheres;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> base;
      Pyramid(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
  };
}


#endif //AT3_PYRAMID_H
