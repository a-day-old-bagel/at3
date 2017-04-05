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
#include <btBulletDynamicsCommon.h>

namespace at3 {

  KalmanSystem::KalmanSystem(State *state) : System(state), distr(0.f, 10.f) {

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
      // Add a little bit of random acceleration to the correct velocity
      btVector3 vel = physics->rigidBody->getLinearVelocity() + elapsed * btVector3(distr(gen), distr(gen), distr(gen));
      // Add the erroneous velocity into the believed position
      glm::vec3 newBelievedPosition = kalman->believedPos + elapsed * kalman->believedVel;
      kalman->believedVel = glm::vec3(vel.x(), vel.y(), vel.z());
      
      Debug::drawLine(*state, initPos, currentPos, {1.f, 1.f, 0.f});
      Debug::drawLine(*state, kalman->believedPos, newBelievedPosition, {0.f, 1.f, 1.f});
      
      kalman->previousTransform = placement->mat;
      kalman->believedPos = newBelievedPosition;
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
    kalman->believedPos = glm::vec3{placement->mat[3][0], placement->mat[3][1], placement->mat[3][2]};
    btVector3 vel = physics->rigidBody->getLinearVelocity();
    kalman->believedVel = glm::vec3(vel.x(), vel.y(), vel.z());
    lastTime = SDL_GetTicks();
  }
}