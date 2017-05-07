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

#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

#include "ezecs.hpp"
#include "dualityInterface.h"
#include "game.h"

#include "ecsSystem_movement.h"
#include "ecsSystem_controls.h"
#include "ecsSystem_physics.h"
#include "ecsSystem_ai.h"

#include "duneBuggy.h"
#include "pyramid.h"
#include "debugStuff.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "TemplateArgumentsIssues"

using namespace at3;
using namespace ezecs;

class CruisinGame : public Game<State, DualityInterface> {
  private:
    std::shared_ptr<SkyBox_>                  mpSkybox;
    std::shared_ptr<TerrainObject_>           mpTerrain;
    std::shared_ptr<Pyramid>                  mpPyramid;
    std::shared_ptr<DuneBuggy>                mpDuneBuggy;
    std::vector<std::shared_ptr<DuneBuggy>>   mvpSweepers;
    std::vector<std::shared_ptr<MeshObject_>> mvpSweeperTargets;

    DualityInterface  mDualityInterface;
    ControlSystem     mControlSystem;
    MovementSystem    mMovementSystem;
    PhysicsSystem     mPhysicsSystem;
    AiSystem          mAiSystem;
    
  public:

    std::shared_ptr<DebugStuff> mpDebugStuff;
    eventHandlerFunction mSystemsHandlerDlgt;
    bool mQuit = false;

    CruisinGame(int argc, char **argv)
        : Game(argc, argv, "at3"),
          mDualityInterface(&state),
          mControlSystem(&state),
          mMovementSystem(&state),
          mPhysicsSystem(&state),
          mAiSystem(&state, -200.f, 200.f, -200.f, 200.f, 0.f) {
      // link the ecs and the scene graph together
      SceneObject_::linkEcs(mDualityInterface);
      // assign the systems' event handler function
      mSystemsHandlerDlgt = std::bind( &CruisinGame::systemsHandler, this, std::placeholders::_1 );
    }

    EzecsResult init() {

      rayFuncType rayFunc = std::bind( &PhysicsSystem::rayTest, &mPhysicsSystem,
                                       std::placeholders::_1, std::placeholders::_2 );

      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= mControlSystem.init();
      initSuccess &= mPhysicsSystem.init();
      initSuccess &= mMovementSystem.init();
      initSuccess &= mAiSystem.init();
      assert(initSuccess);

      // an identity matrix
      glm::mat4 ident;

      // a terrain
      TerrainObject_::initTextures();
      mpTerrain = std::shared_ptr<TerrainObject_> (
          new TerrainObject_(ident, -5000.f, 5000.f, -5000.f, 5000.f, -200, 0));
      this->scene.addObject(mpTerrain);

      // a buggy
      glm::mat4 buggyMat = glm::translate(ident, { 0.f, -290.f, 0.f });
      mpDuneBuggy = std::shared_ptr<DuneBuggy> (
          new DuneBuggy(state, scene, buggyMat));

      // some sweeper buggies
      for (int i = 0; i < CParams::iNumSweepers; ++i) {
        glm::mat4 transform = mAiSystem.randTransformWithinDomain(rayFunc, 200.f, 5.f);
        mvpSweepers.push_back(std::shared_ptr<DuneBuggy>(
            new DuneBuggy(state, scene, transform)));
        entityId sweeperId = mvpSweepers.back()->mpChassis->getId();
        state.add_SweeperAi(sweeperId);
      }
      // some targets for the sweeper buggies
      for (int i = 0; i < CParams::iNumMines; ++i) {
        glm::mat4 transform = mAiSystem.randTransformWithinDomain(rayFunc, 200.f, 5.f);
        mvpSweeperTargets.push_back(std::shared_ptr<MeshObject_>(
            new MeshObject_("assets/models/sphere.dae", "assets/textures/pyramid_flames.png",
                            transform, MeshObject_::FULLBRIGHT)));
        entityId targetId = mvpSweeperTargets.back()->getId();
        state.add_SweeperTarget(targetId);
        btVector3 boxDims(1.f, 1.f, 1.f);
        state.add_Physics(mvpSweeperTargets.back()->getId(), 10.f, &boxDims, Physics::BOX);
        Physics *physics;
        state.get_Physics(mvpSweeperTargets.back()->getId(), &physics);
        physics->rigidBody->applyCentralImpulse({0.f, 0.f, 1.f});
        physics->rigidBody->setDamping(physics->rigidBody->getLinearDamping(), 0.9f);
        physics->rigidBody->setFriction(0.8f);
        scene.addObject(mvpSweeperTargets.back());
      }
      mAiSystem.beginSimulation(rayFunc);

      // a flying pyramid
      glm::mat4 pyramidMat = glm::translate(ident, { 0.f, 0.f, 5.f });
      mpPyramid = std::shared_ptr<Pyramid> (
          new Pyramid(state, scene, pyramidMat));

      // a skybox-like background (but better than a literal sky box)
      mpSkybox = std::shared_ptr<SkyBox_> (
          new SkyBox_());
      this->scene.addObject(mpSkybox);
      LoadResult loaded = mpSkybox->useCubeMap("sea", "png");
      assert(loaded == LOAD_SUCCESS);

      this->setCamera(mpPyramid->mpCamera->mpCamera);

      // some debug-draw features...
      mpDebugStuff = std::shared_ptr<DebugStuff> (
          new DebugStuff(scene, &mPhysicsSystem));

      return EZECS_SUCCESS;
    }
    void deInit() {
      mPhysicsSystem.deInit();
    }
    bool systemsHandler(SDL_Event& event) {
      if (mControlSystem.handleEvent(event)) {
        return true; // handled it here
      }
      if (mPhysicsSystem.handleEvent(event)) {
        return true; // handled it here
      }
      switch (event.type) {
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_E: {
              mpPyramid->spawnSphere();
            } break;
            case SDL_SCANCODE_1: {
              setCamera(mpPyramid->mpCamera->mpCamera);
            } break;
            case SDL_SCANCODE_2: {
              setCamera(mpDuneBuggy->mpCamera->mpCamera);
            } break;
            case SDL_SCANCODE_3:
            case SDL_SCANCODE_4:
            case SDL_SCANCODE_5:
            case SDL_SCANCODE_6:
            case SDL_SCANCODE_7:
            case SDL_SCANCODE_8:
            case SDL_SCANCODE_9:
            case SDL_SCANCODE_0: {
              setCamera(mvpSweepers[event.key.keysym.scancode - SDL_SCANCODE_3]->mpCamera->mpCamera);
            } break;
            case SDL_SCANCODE_RCTRL: {
              mpDuneBuggy->tip();
            } break;
            default: return false; // could not handle it here
        } break;
        default: return false; // could not handle it here
      }
      return true; // handled it here
    }
    void tick(float dt) {
      mControlSystem.setWorldView(getCamera()->lastWorldViewQueried);
      mControlSystem.tick(dt);
      mAiSystem.tick(dt);
      mPhysicsSystem.tick(dt);
      mMovementSystem.tick(dt);

      mpPyramid->resizeFire();
    }
};

void mainLoop(void *instance) {
  CruisinGame *game = (CruisinGame *) instance;
  float dt;
  bool keepGoing = game->mainLoop(game->mSystemsHandlerDlgt, dt);
  if (!keepGoing) {
    game->deInit();
    game->mQuit = true;
    exit(0);  // For future compatibility with Emscripten
  }
  game->tick(dt);
}

int main(int argc, char **argv) {
  CruisinGame game(argc, argv);
  EzecsResult status = game.init();
  if (status.isError()) { fprintf(stderr, "%s", status.toString().c_str()); }

  // snazzy tunes
//  game.debugStuff->queueMusic();

  while (!game.mQuit) {
    mainLoop(&game);
  }
  return 0;
}

#pragma clang diagnostic pop