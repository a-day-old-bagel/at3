
#pragma once

#include <vector>
#include "ecsInterface.hpp"
#include "scene.hpp"
#include "debug.hpp"
#include "ecsSystem_physics.hpp"

namespace at3 {
  typedef std::function<void(glm::vec3&, glm::vec3&, glm::vec3&)> lineDrawFuncType;
  class DebugStuff {
    private:
      Scene_* scene;
      std::shared_ptr<BulletDebug_> bulletDebug;
      rtu::topics::Subscription lineDrawRequestSubscription;
      void fulfillDrawLineRequest(void* args);
      void addToScene();
    public:
      DebugStuff(Scene_ &scene, PhysicsSystem* physicsSystem);
      ~DebugStuff();
      void queueMusic();
      lineDrawFuncType getLineDrawFunc();
  };
}
