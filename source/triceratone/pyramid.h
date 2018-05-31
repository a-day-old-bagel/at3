//
// Created by volundr on 4/27/17.
//

#pragma once

#include <vector>
#include "ecsInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class Pyramid {
    private:
      std::shared_ptr<MeshObject_> top, thrusters, fire;
      std::vector<std::shared_ptr<MeshObject_>> spheres;
      ezecs::State* state;
      Scene_* scene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> base;
      std::shared_ptr<ThirdPersonCamera_> camera;
      ezecs::entityId ctrlId;
      ezecs::entityId camGimbalId;

      Pyramid(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
      void makeActiveControl(void* nothing);
  };
}
