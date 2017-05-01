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
    std::shared_ptr<PerspectiveCamera_> m_camera;
    std::shared_ptr<SceneObject_> m_camGimbal;
    std::shared_ptr<SkyBox_> m_skyBox;
    std::shared_ptr<TerrainObject_> m_terrain;
    std::shared_ptr<DuneBuggy> duneBuggy;
    std::shared_ptr<Pyramid> pyramid;

    DualityInterface dualityInterface;
    ControlSystem controlSystem;
    MovementSystem movementSystem;
    PhysicsSystem physicsSystem;
    AiSystem aiSystem;
    
  public:
    std::shared_ptr<DebugStuff> debugStuff;
    eventHandlerFunction systemsHandlerDlgt;
    bool quit = false;
    CruisinGame(int argc, char **argv)
        : Game(argc, argv, "at3"),
          dualityInterface(&state),
          controlSystem(&state),
          movementSystem(&state),
          physicsSystem(&state),
          aiSystem(&state) {
      // link the ecs and the scene graph together
      SceneObject_::linkEcs(dualityInterface);
      // assign the systems' event handler function
      systemsHandlerDlgt = std::bind( &CruisinGame::systemsHandler, this, std::placeholders::_1 );
    }

    EzecsResult init() {
      // Initialize the systems
      bool initSuccess = true;
      initSuccess &= controlSystem.init();
      initSuccess &= physicsSystem.init();
      initSuccess &= movementSystem.init();
      initSuccess &= aiSystem.init();
      assert(initSuccess);

      // an identity matrix
      glm::mat4 ident;

      // a terrain
      TerrainObject_::initTextures();
      m_terrain = std::shared_ptr<TerrainObject_> (
          new TerrainObject_(ident, -5000.f, 5000.f, -5000.f, 5000.f, -200, 300));
      this->scene.addObject(m_terrain);

      // a buggy
      glm::mat4 buggyMat = glm::translate(ident, { 0.f, -290.f, 0.f });
      duneBuggy = std::shared_ptr<DuneBuggy> (
          new DuneBuggy(state, scene, buggyMat));

      // a flying pyramid
      glm::mat4 pyramidMat = glm::translate(ident, { 0.f, 0.f, 5.f });
      pyramid = std::shared_ptr<Pyramid> (
          new Pyramid(state, scene, pyramidMat));

      // a skybox-like background (but better than a literal sky box)
      m_skyBox = std::shared_ptr<SkyBox_> (
          new SkyBox_());
      this->scene.addObject(m_skyBox);
      LoadResult loaded = m_skyBox->useCubeMap("sea", "png");
      assert(loaded == LOAD_SUCCESS);

      // a camera with a gimbal for a third-person camera setup
      glm::mat4 cameraMat =
          glm::rotate(glm::translate(ident, {0.f, -6.f, 1.0f}), (float) M_PI * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
      m_camera = std::shared_ptr<PerspectiveCamera_> (
          new PerspectiveCamera_(fovy(), 1.0f, 10000.0f, cameraMat));
      m_camGimbal = std::shared_ptr<SceneObject_> (
          new SceneObject_());

      // add mouse controls to the camera gimbal so that it rotates with the mouse
      entityId gimbalId = m_camGimbal->getId();
      state.add_Placement(gimbalId, {
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, 0, 1, 1  // at (0, 0, 1)
      });
      state.add_MouseControls(gimbalId, false, false);

      // attach the camera to the camera gimbal
      m_camGimbal->addChild(m_camera);

      // tell the scene to use the camera
      this->setCamera(m_camera);

      // the third-person camera gimbal will be attached to the pyramid initially
      pyramid->base->addChild(m_camGimbal, SceneObject_::TRANSLATION_ONLY);

      // some debug-draw features...
      debugStuff = std::shared_ptr<DebugStuff> (
          new DebugStuff(scene, &physicsSystem));

      return EZECS_SUCCESS;
    }
    void deInit() {
      physicsSystem.deInit();
    }
    bool systemsHandler(SDL_Event& event) {
      if (controlSystem.handleEvent(event)) {
        return true; // handled it here
      }
      if (physicsSystem.handleEvent(event)) {
        return true; // handled it here
      }
      switch (event.type) {
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_E: {
              pyramid->spawnSphere();

            } break;
            case SDL_SCANCODE_C: {
              if (pyramid->base->hasChild(m_camGimbal.get())) {
                pyramid->base->removeChild(m_camGimbal.get());
                duneBuggy->chassis->addChild(m_camGimbal, SceneObject_::TRANSLATION_ONLY);
              } else {
                duneBuggy->chassis->removeChild(m_camGimbal.get());
                pyramid->base->addChild(m_camGimbal, SceneObject_::TRANSLATION_ONLY);
              }
            } break;
            case SDL_SCANCODE_RCTRL: {
              duneBuggy->tip();
            } break;
            default: return false; // could not handle it here
        } break;
        default: return false; // could not handle it here
      }
      return true; // handled it here
    }
    void tick(float dt) {
      controlSystem.setWorldView(m_camera->lastWorldViewQueried);
      controlSystem.tick(dt);
      aiSystem.tick(dt);
      physicsSystem.tick(dt);
      movementSystem.tick(dt);

      pyramid->resizeFire();
    }
};

void mainLoop(void *instance) {
  CruisinGame *game = (CruisinGame *) instance;
  float dt;
  bool keepGoing = game->mainLoop(game->systemsHandlerDlgt, dt);
  if (!keepGoing) {
    game->deInit();
    game->quit = true;
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

  while (!game.quit) {
    mainLoop(&game);
  }
}

#pragma clang diagnostic pop