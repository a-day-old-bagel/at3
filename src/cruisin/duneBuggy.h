//
// Created by volundr on 4/27/17.
//

#ifndef AT3_DUNEBUGGY_H
#define AT3_DUNEBUGGY_H

#include <vector>
#include "dualityInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class DuneBuggy {
    private:
      ezecs::entityId chassisId;
      std::vector<std::shared_ptr<MeshObject_>> wheels;
      ezecs::State* state;
      Scene_* scene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> chassis;
      DuneBuggy(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      void tip();
  };
}


#endif //AT3_DUNEBUGGY_H
