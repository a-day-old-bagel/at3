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
#ifndef ECSSYSTEM_KALMAN_H
#define ECSSYSTEM_KALMAN_H

#include <random>
#include <armadillo>
#include "ezecs.hpp"

using namespace ezecs;

namespace at3 {
  class KalmanSystem : public System<KalmanSystem> {
      uint32_t lastTime;
      bool isRunning = false;
      std::default_random_engine gen;
      std::normal_distribution<float> distr;
      arma::mat prediction;
      arma::mat predictionTranspose;

      void beginKalman(const entityId& id);

    public:
      std::vector<compMask> requiredComponents = {
          KALMAN | PHYSICS
      };
      KalmanSystem(State* state);
      bool onInit();
      void onTick(float dt);
      bool onDiscover(const entityId& id);
      bool onForget(const entityId& id);
      bool handleEvent(SDL_Event &event);
  };
}

#endif //ECSSYSTEM_KALMAN_H
