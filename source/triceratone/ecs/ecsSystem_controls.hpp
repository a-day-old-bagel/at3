
#pragma once

#include <memory>
#include <SDL.h>
#include "ezecs.hpp"
#include "topics.hpp"
#include "eventResponseMap.hpp"

using namespace ezecs;

namespace at3 {
  class EntityAssociatedERM;
  class ControlSystem : public System<ControlSystem> {
      glm::mat4 lastKnownWorldView = glm::mat4(1.f);
      glm::mat3 lastKnownCylCtrlRot = glm::mat3(1.f);
      bool lookInfoIsFresh = false;

      rtu::topics::Subscription updateWvMatSub;
      rtu::topics::Subscription switchToWalkCtrlSub;
      rtu::topics::Subscription switchToPyramidCtrlSub;
      rtu::topics::Subscription switchToTrackCtrlSub;
      rtu::topics::Subscription switchToFreeCtrlSub;
      std::unique_ptr<EntityAssociatedERM> currentCtrlKeys;

      rtu::topics::Subscription switchToMouseCtrlSub;
      std::unique_ptr<EntityAssociatedERM> currentCtrlMouse;

      void updateLookInfos();
      void setWorldView(void* p_wv);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);
      void switchToFreeCtrl(void *id);

    public:
      std::vector<compMask> requiredComponents = {
              PYRAMIDCONTROLS,
              TRACKCONTROLS,
              PLAYERCONTROLS,
              FREECONTROLS
      };
      ControlSystem(State* state);
      ~ControlSystem() override;
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
  class EntityAssociatedERM : public rtu::EventResponseMap {
    protected:
      State *state;
      entityId id;
    public:
      EntityAssociatedERM(State *state, entityId id);
      virtual ~EntityAssociatedERM() = default;
      virtual void tick() = 0;
      entityId getId();
  };
}
