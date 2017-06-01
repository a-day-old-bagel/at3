//
// Created by volundr on 4/27/17.
//

#ifndef AT3_DEBUGSTUFF_H
#define AT3_DEBUGSTUFF_H

#include <vector>
#include "dualityInterface.h"
#include "scene.h"
#include "debug.h"
#include "ecsSystem_physics.h"

namespace at3 {
  typedef std::function<void(glm::vec3&, glm::vec3&, glm::vec3&)> lineDrawFuncType;
  class DebugStuff {
    private:
      Scene_* mpScene;
      std::shared_ptr<BulletDebug_> mpBulletDebug;
      void addToScene();
    public:
      DebugStuff(Scene_ &scene, PhysicsSystem* physicsSystem);
      void queueMusic();
      lineDrawFuncType getLineDrawFunc();
  };
}


#endif //AT3_DEBUGSTUFF_H