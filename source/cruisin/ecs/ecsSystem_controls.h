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

#include <SDL.h>
#include "ezecs.hpp"
#include "topics.h"

using namespace ezecs;

namespace at3 {
  class ControlSystem : public System<ControlSystem> {
      std::vector<SDL_Event> queuedEvents;
      glm::mat4 lastKnownWorldView;
      glm::vec3 lastKnownLookVec;
      glm::mat3 lastKnownHorizCtrlRot;
      bool lookInfoIsFresh = false;
      topics::Subscription wvSub;

      void updateLookInfos();

    public:
      std::vector<compMask> requiredComponents = {
              MOUSECONTROLS,
              PYRAMIDCONTROLS,
              TRACKCONTROLS,
              PLAYERCONTROLS
      };
      ControlSystem(State* state);
      bool onInit();
      void onTick(float dt);
      bool handleEvent(SDL_Event& event);
//      void setWorldView(glm::mat4 &wv);
      void setWorldView(void* p_wv);
  };
}

#endif //ECSSYSTEM_WASDCONTROLS_H
