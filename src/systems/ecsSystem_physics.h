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
#ifndef LD2016_ECSSYSTEM_PHYSICS_H
#define LD2016_ECSSYSTEM_PHYSICS_H

#include <epoxy/gl.h>
#include <btBulletDynamicsCommon.h>
#include <LinearMath/btIDebugDraw.h>
#include "ezecs.hpp"
#include "sceneObject.h"
#include "bulletDebug.h"

using namespace ezecs;

// TODO: figure out how to use bullet to simulate objects without collision components
namespace at3 {

  /*
   * The physics system
   */
  class PhysicsSystem : public System<PhysicsSystem> {
      /* Global physics data structures */
      btDispatcher *dispatcher;
      btBroadphaseInterface *broadphase;
      btConstraintSolver *solver;
      btCollisionConfiguration *collisionConfiguration;
      btDynamicsWorld *dynamicsWorld;
      btVehicleRaycaster * vehicleRaycaster;
      bool debugDrawMode = false;

    public:
      std::vector<compMask> requiredComponents = {
              PHYSICS,
              PHYSICS | PYRAMIDCONTROLS,
              PHYSICS | TERRAIN,
              PHYSICS | TRACKCONTROLS
      };
      PhysicsSystem(State* state);
      bool onInit();
      void onTick(float dt);
      void deInit();
      bool onDiscover(const entityId& id);
      bool onForget(const entityId& id);
      bool onDiscoverTerrain(const entityId& id);
      bool onDiscoverTrackControls(const entityId& id);
      bool onForgetTrackControls(const entityId& id);
      bool handleEvent(SDL_Event& event);
      void setDebugDrawer(std::shared_ptr<BulletDebug> debug);
      // TODO: add removal logic for vehicles and wheels
  };
}

#endif //LD2016_ECSSYSTEM_PHYSICS_H
