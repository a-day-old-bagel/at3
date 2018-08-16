
#pragma once

//#include <vector>
//#include "interface.hpp"
//#include "ezecs.hpp"
//#include "sceneTree.hpp"
//#include "vkc.hpp"

#include "interface.hpp"

namespace at3 {
  namespace Pyramid {
    ezecs::entityId create(ezecs::State &state, const glm::mat4 &transform);
    void broadcastLatest(ezecs::State &state, EntityComponentSystemInterface &ecs);
    void switchTo(ezecs::State &state, const ezecs::entityId & id);
    const ezecs::TransformFunctionDescriptor & getTopTransFuncDesc();
    const ezecs::TransformFunctionDescriptor & getFireTransFuncDesc();
  }



//  class Pyramid {
//    private:
//
//      ezecs::State* state;
//      std::shared_ptr<EntityComponentSystemInterface> ecs;
//
//      ezecs::entityId topId, thrustersId, fireId;
//      std::vector<ezecs::entityId> spheres;
//
//      void addToScene();
//
//    public:
//
//      ezecs::entityId bottomId;
//      ezecs::entityId camId;
//      ezecs::entityId camGimbalId;
//
//      Pyramid(ezecs::State &state, std::shared_ptr<EntityComponentSystemInterface> ecs, glm::mat4 &transform);
//      void makeActiveControl(void* nothing);
//      const static ezecs::TransformFunctionDescriptor & getTopTransFuncDesc();
//      const static ezecs::TransformFunctionDescriptor & getFireTransFuncDesc();
//
//  };
}
