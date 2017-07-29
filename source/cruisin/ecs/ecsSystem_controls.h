/*
 * Copyright (c) 2016 Galen Cochrane
 * Galen Cochrane <galencochrane@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef ECSSYSTEM_WASDCONTROLS_H
#define ECSSYSTEM_WASDCONTROLS_H

#include <memory>
#include <SDL.h>
#include "ezecs.hpp"
#include "topics.hpp"
#include "activeControl.h"

using namespace ezecs;

namespace at3 {
  class SwitchableControls;
  class ControlSystem : public System<ControlSystem> {
      glm::mat4 lastKnownWorldView;
      glm::vec3 lastKnownLookVec;
      glm::mat3 lastKnownHorizCtrlRot;
      bool lookInfoIsFresh = false;

      rtu::topics::Subscription updateWvMatSub;
      rtu::topics::Subscription switchToWalkCtrlSub;
      rtu::topics::Subscription switchToPyrmCtrlSub;
      rtu::topics::Subscription switchToTrakCtrlSub;
      std::unique_ptr<SwitchableControls> currentCtrlKeys;

      rtu::topics::Subscription switchToMousCtrlSub;
      std::unique_ptr<SwitchableControls> currentCtrlMous;

      void updateLookInfos();
      void setWorldView(void* p_wv);
      void switchToMousCtrl(void *id);
      void switchToWalkCtrl(void* id);
      void switchToPyrmCtrl(void* id);
      void switchToTrakCtrl(void *id);

    public:
      std::vector<compMask> requiredComponents = {
              PYRAMIDCONTROLS,
              TRACKCONTROLS,
              PLAYERCONTROLS
      };
      ControlSystem(State* state);
      bool onInit();
      void onTick(float dt);
      bool handleEvent(SDL_Event& event);
  };

  /*
   * SwitchableControls is a base class for various control objects declared and defined in
   * ecsSystem_controls.cpp. It's purpose is to allow for user input to be routed to a certain
   * active control interface or shared between control interfaces if necessary.
   * ControlSystem has a single unique_ptr that points to one of these at a time, corresponding
   * to the entity currently being controlled by the player.
   */
  class SwitchableControls : public ActiveControl {
    protected:
      State *state;
      entityId id;
    public:
      SwitchableControls(State *state, const entityId id) : state(state), id(id) { }
      entityId getId() { return id; }
  };
}

#endif //ECSSYSTEM_WASDCONTROLS_H
