
#ifndef ECSSYSTEM_WASDCONTROLS_H
#define ECSSYSTEM_WASDCONTROLS_H

#include <memory>
#include <SDL.h>
#include "ezecs.hpp"
#include "topics.hpp"
#include "eventResponseMap.h"

using namespace ezecs;

namespace at3 {
  class EntityAssociatedERM;
  class ControlSystem : public System<ControlSystem> {
      glm::mat4 lastKnownWorldView;
      glm::vec3 lastKnownLookVec;
      glm::mat3 lastKnownHorizCtrlRot;
      bool lookInfoIsFresh = false;

      rtu::topics::Subscription updateWvMatSub;
      rtu::topics::Subscription switchToWalkCtrlSub;
      rtu::topics::Subscription switchToPyrmCtrlSub;
      rtu::topics::Subscription switchToTrakCtrlSub;
      std::unique_ptr<EntityAssociatedERM> currentCtrlKeys;

      rtu::topics::Subscription switchToMousCtrlSub;
      std::unique_ptr<EntityAssociatedERM> currentCtrlMous;

      void updateLookInfos();
      void setWorldView(void* p_wv);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);

    public:
      std::vector<compMask> requiredComponents = {
              PYRAMIDCONTROLS,
              TRACKCONTROLS,
              PLAYERCONTROLS
      };
      ControlSystem(State* state);
      bool onInit();
      void onTick(float dt);
  };

  /*
   * EntityAssociatedERM is a base class for various control objects declared and defined in
   * ecsSystem_controls.cpp. It's purpose is to allow for user input to be routed to a certain
   * active control interface (belonging to an entity) or shared between control interfaces if necessary.
   * ControlSystem has a single unique_ptr that points to one of these at a time, corresponding
   * to the entity currently being controlled by the player.
   */
  class EntityAssociatedERM : public EventResponseMap {
    protected:
      State *state;
      entityId id;
    public:
      EntityAssociatedERM(State *state, entityId id);
      entityId getId();
  };
}

#endif //ECSSYSTEM_WASDCONTROLS_H