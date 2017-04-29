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
#include <SDL_timer.h>
#include "debug.h"
#include "ecsSystem_kalman.h"

namespace at3 {

  KalmanSystem::KalmanSystem(State *state)
      : System(state), distr(0.f, 10.f), prediction(6, 6), predictionTranspose(6, 6)
  {
    prediction.eye();
    predictionTranspose.eye();
  }
  bool KalmanSystem::onInit() {
    lastTime = SDL_GetTicks();
    return true;
  }
  void KalmanSystem::onTick(float dt) {
    if (!isRunning) { return; }
    
    uint32_t nowTime = SDL_GetTicks();
    uint32_t deltaTime = nowTime - lastTime;
    if (deltaTime < 100) { return; }
    lastTime = nowTime;
    
    for (auto id : registries[0].ids) {
      Kalman* kalman;
      state->get_Kalman(id, &kalman);
      Placement* placement;
      state->get_Placement(id, &placement);
      Physics* physics;
      state->get_Physics(id, &physics);
      
      glm::vec3 initPos{
          kalman->previousTransform[3][0],
          kalman->previousTransform[3][1],
          kalman->previousTransform[3][2]
      };
      glm::vec3 currentPos{
          placement->mat[3][0],
          placement->mat[3][1],
          placement->mat[3][2]
      };
      
      float elapsed = (deltaTime * 0.001f);
      float noiseX = distr(gen);
      float noiseY = distr(gen);
      float noiseZ = distr(gen);
      btVector3 noiseVec(noiseX, noiseY, noiseZ);

      /*prediction[18] = elapsed;
      prediction[25] = elapsed;
      prediction[32] = elapsed;
      predictionTranspose[3] = elapsed;
      predictionTranspose[10] = elapsed;
      predictionTranspose[17] = elapsed;

      kalman->covariance = prediction * kalman->covariance * predictionTranspose;
      kalman->covariance.print(std::cout);
      std::cout << std::endl;*/
      
      btVector3 realVel = physics->rigidBody->getLinearVelocity();
      btVector3 realAccel = (realVel - kalman->realVel) / elapsed;
      btVector3 measuredAccel = realAccel + elapsed * noiseVec;

//      arma::vec state0 = {
//          kalman->believedPos.x(), kalman->believedPos.y(), kalman->believedPos.z(),
//          kalman->believedVel.x(), kalman->believedVel.y(), kalman->believedVel.z()
//      };
//      arma::vec state1(6);

      btVector3 newBelievedVel = kalman->believedVel + elapsed * measuredAccel;

      // midpoint integration approximation for position update step
      btVector3 believedTranslation = elapsed * ((kalman->believedVel + newBelievedVel) * 0.5);
      btVector3 newBelievedPosition = kalman->believedPos + believedTranslation;
      btVector3 kTranslation = elapsed * ((kalman->kVel + realVel) * 0.5) + noiseVec * 0.001;
      btVector3 newKPos = kalman->kPos + kTranslation;

      // draw the real path
      Debug::drawLine(*state, initPos, currentPos, {1.f, 1.f, 0.f});
      // draw the believed path
      Debug::drawLine(*state,
                      {kalman->believedPos.x(), kalman->believedPos.y(), kalman->believedPos.z()},
                      {newBelievedPosition.x(), newBelievedPosition.y(), newBelievedPosition.z()},
                      {0.f, 1.f, 1.f});
      // draw the k path
      Debug::drawLine(*state,
                      {kalman->kPos.x(), kalman->kPos.y(), kalman->kPos.z()},
                      {newKPos.x(), newKPos.y(), newKPos.z()},
                      {1.f, 1.f, 1.f});
      
      kalman->kPos = newKPos;
      kalman->kVel = realVel;
      
      kalman->previousTransform = placement->mat;
      kalman->believedPos = newBelievedPosition;
      kalman->believedVel = newBelievedVel;
      kalman->realVel = realVel;
    }
  }
  bool KalmanSystem::onDiscover(const entityId& id) {
    beginKalman(id);
    return true;
  }
  bool KalmanSystem::onForget(const entityId& id) {
    return true;
  }
  bool KalmanSystem::handleEvent(SDL_Event& event) {
    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_F: {
            isRunning = !isRunning;
            if (isRunning) {
              for (auto id : registries[0].ids) {
                beginKalman(id);
              }
            }
          }
            break;
          default:
            return false; // could not handle it here
        }
        break;
      default:
        return false; // could not handle it here
    }
    return true; // handled it here
  }
  void KalmanSystem::beginKalman(const entityId& id) {
    Kalman* kalman;
    state->get_Kalman(id, &kalman);
    Placement* placement;
    state->get_Placement(id, &placement);
    Physics* physics;
    state->get_Physics(id, &physics);
    kalman->previousTransform = placement->mat;
    kalman->believedPos = btVector3{placement->mat[3][0], placement->mat[3][1], placement->mat[3][2]};
    kalman->realVel = physics->rigidBody->getLinearVelocity();
    kalman->believedVel = kalman->realVel;
    kalman->kPos = kalman->believedPos;
    kalman->kVel = kalman->believedVel;
    lastTime = SDL_GetTicks();
  }
}