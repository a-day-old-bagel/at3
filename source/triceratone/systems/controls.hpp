
#pragma once

#include <memory>
#include <SDL.h>
#include "ezecs.hpp"
#include "topics.hpp"
#include "eventResponseMap.hpp"
#include "interface.hpp"

using namespace ezecs;

namespace at3 {
  class EntityAssociatedERM;
  class ControlSystem : public System<ControlSystem> {

      std::shared_ptr<EntityComponentSystemInterface> ecs;
      std::unique_ptr<EntityAssociatedERM> currentCtrlMouse;
      std::unique_ptr<EntityAssociatedERM> currentCtrlKeys;

      void setEcsInterface(void *ecs);
      void switchToMouseCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyramidCtrl(void *id);
      void switchToTrackCtrl(void *id);
      void switchToFreeCtrl(void *id);

    public:
      std::vector<compMask> requiredComponents = {
              MOUSECONTROLS,
              PYRAMIDCONTROLS,
              TRACKCONTROLS,
              WALKCONTROLS,
              FREECONTROLS
      };
      explicit ControlSystem(State* state);
      ~ControlSystem() override;
      bool onInit();
      void onTick(float dt);
      bool onDiscoverMouseControls(const entityId &id);
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
      entityId getId();
  };
}
