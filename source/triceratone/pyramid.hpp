
#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "ezecs.hpp"
#include "sceneTree.hpp"
#include "vkc.hpp"

namespace at3 {
  class Pyramid {
    private:

//      std::shared_ptr<Mesh> top, thrusters, fire;
//      std::vector<std::shared_ptr<Mesh>> spheres;
      std::shared_ptr<Object> top, thrusters, fire;
      std::vector<std::shared_ptr<Object>> spheres;

      ezecs::State* state;
      Scene* scene;
      vkc::VulkanContext<EntityComponentSystemInterface>* vkc;
      void addToScene();
    public:

//      std::shared_ptr<Mesh> base;
//      std::shared_ptr<ThirdPersonCamera> camera;
      std::shared_ptr<Object> base;
      std::shared_ptr<Object> camera;
      std::shared_ptr<Object> gimbal;

      ezecs::entityId bottomId;
      ezecs::entityId camId;
      ezecs::entityId camGimbalId;

      Pyramid(ezecs::State &state, vkc::VulkanContext<EntityComponentSystemInterface> *context, Scene &scene,
                glm::mat4 &transform);
      void resizeFire();
      ezecs::entityId spawnSphere(bool shoot = false);
      void dropSphere();
      void shootSphere();

//      std::shared_ptr<PerspectiveCamera> getCamPtr();

      void makeActiveControl(void* nothing);
  };
}
