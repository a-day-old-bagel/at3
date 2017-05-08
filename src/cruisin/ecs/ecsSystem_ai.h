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

#include <functional>
#include <SDL.h>
#include "dualityInterface.h"
#include "ezecs.hpp"

#include "CGenAlg.h"
#include "CNeuralNet.h"
#include "CParams.h"
#include "SVector2D.h"
#include "utils.h"

using namespace ezecs;

namespace at3 {
  typedef std::function<btCollisionWorld::ClosestRayResultCallback(btVector3 &, btVector3 &)> rayFuncType;
  typedef std::function<void(glm::vec3&, glm::vec3&, glm::vec3&)> lineDrawFuncType;

  void nullDraw(glm::vec3&, glm::vec3&, glm::vec3&);

  class AiSystem : public System<AiSystem> {
      std::vector<entityId> participants;
      std::vector<entityId> lateComers;
      std::vector<SGenome> population;
      CParams params;
      CGenAlg* geneticAlgorithm = NULL;
      rayFuncType rayFunc;
      lineDrawFuncType lineDrawFunc = &nullDraw;
      uint32_t lastTime;
      float minX, maxX, minY, maxY, spawnHeight;
      int numWeightsInNN, generationCount = 0;
      bool simulationStarted = false;

    public:
      std::vector<compMask> requiredComponents = {
              SWEEPERAI,
              SWEEPERTARGET
      };
      AiSystem(State* state, float minX, float maxX, float minY, float maxY, float spawnHeight);
      ~AiSystem();
      bool onInit();
      void onTick(float dt);
      bool handleEvent(SDL_Event& event);
      void beginSimulation(rayFuncType &rayFunc);
      bool onDiscover(const entityId &id);
      bool onForget(const entityId &id);
      glm::vec2 randVec2WithinDomain();
      glm::vec2 randVec2WithinDomain(float scale);
      glm::mat4 randTransformWithinDomain(rayFuncType& rayFunc, float rayLen, float offsetHeight, float scale = 1.f);
      void setLineDrawFunc(lineDrawFuncType &&func);
  };
}

#endif //ECSSYSTEM_AISYSTEM_H
