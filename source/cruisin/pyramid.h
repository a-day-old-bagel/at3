//
// Created by volundr on 4/27/17.
//

#pragma once

#include <vector>
#include "dualityInterface.h"
#include "ezecs.hpp"
#include "scene.h"
#include "meshObject.h"

namespace at3 {
  class Pyramid {
    private:
      std::shared_ptr<MeshObject_> mpTop, mpThrusters, mpFire;
      std::vector<std::shared_ptr<MeshObject_>> mvpSpheres;
      ezecs::State* mpState;
      Scene_* mpScene;
      void addToScene();
    public:
      std::shared_ptr<MeshObject_> mpBase;
      std::shared_ptr<ThirdPersonCamera_> mpCamera;
      ezecs::entityId ctrlId;

      Pyramid(ezecs::State &state, Scene_ &scene, glm::mat4 &transform);
      void resizeFire();
      void spawnSphere();
      std::shared_ptr<PerspectiveCamera_> getCamPtr();
  };
}
