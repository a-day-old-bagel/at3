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
#ifndef ECSSYSTEM_AISYSTEM_H
#define ECSSYSTEM_AISYSTEM_H

#include <SDL.h>
#include "dualityInterface.h"
#include "ezecs.hpp"
#include "sweepers.h"

using namespace ezecs;

namespace at3 {
  class AiSystem : public System<AiSystem> {
      std::vector<entityId> participants;
      std::vector<entityId> lateComers;
      std::vector<SGenome> population;
      std::vector<std::shared_ptr<MeshObject_>>* targets;
      std::vector<SVector2D> vecTargets;
      bool simulationStarted = false;
      CParams params;
      CGenAlg* geneticAlgorithm = NULL;
      int numWeightsInNN;
      int ticks = 0;

    public:
      std::vector<compMask> requiredComponents = {
              SWEEPERAI
      };
      AiSystem(State* state);
      ~AiSystem();
      bool onInit();
      void onTick(float dt);
      bool handleEvent(SDL_Event& event);
      void beginSimulation(std::vector<std::shared_ptr<MeshObject_>> *targets);
      bool onDiscover(const entityId &id);
      bool onForget(const entityId &id);
  };
}

#endif //ECSSYSTEM_AISYSTEM_H
