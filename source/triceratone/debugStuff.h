
#pragma once

#include <vector>
#include "ecsInterface.h"
#include "scene.h"
#include "debug.h"
#include "ecsSystem_physics.h"

namespace at3 {
  typedef std::function<void(glm::vec3&, glm::vec3&, glm::vec3&)> lineDrawFuncType;
  class DebugStuff {
    private:
      Scene_* mpScene;
      std::shared_ptr<BulletDebug_> mpBulletDebug;
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
