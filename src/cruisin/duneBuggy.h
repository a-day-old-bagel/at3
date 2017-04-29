//
// Created by volundr on 4/27/17.
//

#ifndef AT3_DUNEBUGGY_H
#define AT3_DUNEBUGGY_H

#include <vector>
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class DuneBuggy {
    private:
      ezecs::entityId chassisId;
      std::vector<std::shared_ptr<MeshObject>> wheels;
      ezecs::State* state;
      Scene* scene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject> chassis;
      DuneBuggy(ezecs::State &state, Scene *scene, glm::mat4 &transform);
      void tip();
  };
}


#endif //AT3_DUNEBUGGY_H
