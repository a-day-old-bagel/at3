/*
 * Copyright (c) 2016 Galen Cochrane, Jonathan Glines
 * Galen Cochrane <galencochrane@gmail.com>
 * Jonathan Glines <jonathan@glines.net>
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
#include "ezecs.hpp"

#include "debug.h"
#include "game.h"
#include "meshObject.h"
#include "scene.h"
#include "perspectiveCamera.h"
#include "skyBox.h"

#include "ecsHelpers.hpp"
#include "ecsSystem_movement.h"
#include "ecsSystem_controls.h"
#include "ecsSystem_physics.h"

#define GET_TIME std::chrono::high_resolution_clock::now();
#define DURATION std::chrono::duration<double, std::milli>
#define CHECK(compOpReturn) EZECS_CHECK_PRINT(EZECS_ERR(compOpReturn))

using namespace ld2016;
using namespace ezecs;

static glm::vec3 pyrFireWiggle(const glm::vec3& initialScale, uint32_t time) {
  return { initialScale.x, initialScale.y, initialScale.z * (1.f + sin(time * 0.1f)) };
}

class PyramidGame : public Game {
  private:
    std::shared_ptr<PerspectiveCamera> m_camera;
    std::shared_ptr<MeshObject> m_pyrBottom, m_pyrTop, m_pyrThrusters, m_pyrFire;
    std::shared_ptr<SceneObject> m_camGimbal;
    std::shared_ptr<SkyBox> m_skyBox;
    ControlSystem controlSystem;
    MovementSystem movementSystem;
    PhysicsSystem physicsSystem;
  public:
    Delegate<bool(SDL_Event&)> systemsHandlerDlgt;
    PyramidGame(int argc, char **argv)
        : Game(argc, argv, "Pyramid Game"), controlSystem(&state), movementSystem(&state), physicsSystem(&state) {
      systemsHandlerDlgt = DELEGATE(&PyramidGame::systemsHandler, this);
    }
    EzecsResult init() {
      assert(controlSystem.init());
      assert(movementSystem.init());
      assert(physicsSystem.init());

      // Populate the graphics scene
      m_camera = std::shared_ptr<PerspectiveCamera> (
          new PerspectiveCamera (
              state,
              80.0f * ((float) M_PI / 180.0f),  // fovy
              0.1f,  // near
              1000.0f,  // far
              glm::vec3(0.0f, -4.f, 0.0f),  // position
              glm::angleAxis(
                  (float) M_PI * 0.5f,
                  glm::vec3(1.0f, 0.0f, 0.0f))
          ));
      m_pyrBottom = std::shared_ptr<MeshObject> (
          new MeshObject(
              state, "assets/models/pyramid_bottom.dae", "assets/textures/pyramid_bottom.png",
              {0.f, 0.f, 100.f}));
      m_pyrTop = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_top.dae", "assets/textures/pyramid_top.png"));
      m_pyrThrusters = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_thrusters.dae", "assets/textures/thrusters.png"));
      m_pyrFire = std::shared_ptr<MeshObject> (
          new MeshObject (
              state, "assets/models/pyramid_thruster_flames.dae", "assets/textures/pyramid_flames.png",
              {0.f, 0.f, -0.85f},
              glm::angleAxis((float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)),
              {0.105f, 0.105f, 0.25f}));
      m_camGimbal = std::shared_ptr<SceneObject> (
          new SceneObject(state));
      m_skyBox = std::shared_ptr<SkyBox> (
          new SkyBox(state));
      float delta = 0.3f;
      for (int i = 0; i < 100; ++i) {
        Debug::drawLine (
            state,
            glm::vec3((float) i * delta, 0.0f, 0.0f),
            glm::vec3((float) i * delta, 1.0f, 0.0f),
            glm::vec3(1.0f, 0.0f, 1.0f));
        Debug::drawLine (
            state,
            glm::vec3(0.0f, (float) i * delta, 0.0f),
            glm::vec3(1.0f, (float) i * delta, 0.0f),
            glm::vec3(1.0f, 0.0f, 1.0f));
      }

      this->scene()->addObject(m_pyrBottom);
      this->scene()->addObject(m_skyBox);
      LoadResult loaded = m_skyBox->useCubeMap("sea", "png");
      assert(loaded == LOAD_SUCCESS);

      m_pyrBottom->addChild(m_camGimbal);
      m_pyrBottom->addChild(m_pyrTop);
      m_pyrBottom->addChild(m_pyrThrusters);
      m_pyrBottom->addChild(m_pyrFire);

      m_camGimbal->addChild(m_camera);
      this->setCamera(m_camera);

      entityId gimbalId = m_camGimbal->getId();
      state.add_Position(gimbalId, {0.f, 0.f, 1.f});
      state.add_Orientation(gimbalId, glm::quat());
      state.add_MouseControls(gimbalId, false, false);

      entityId bottomId = m_pyrBottom->getId();
      state.add_WasdControls(bottomId, gimbalId, WasdControls::ROTATE_ABOUT_Z);
      float sphereRadius = 0.8f;
      state.add_Physics(bottomId, 1.f, &sphereRadius, Physics::SPHERE);
      /*std::vector<float> hullVerts = {
          *//*{ 0.f,  0.f, -0.2f},
          {-.2f, -.2f, 0.f},
          {-.2f,  .2f, 0.f},
          { .2f, -.2f, 0.f},
          { .2f,  .2f, 0.f}*//*
           1.f,  0.f,  0.f,
           0.f,  1.f,  0.f,
           0.f,  0.f,  1.f,
          -1.f,  0.f,  0.f,
           0.f, -1.f,  0.f,
           0.f,  0.f, -1.f
      };
      state.addPhysics(bottomId, 1.f, &hullVerts, Physics::MESH);*/

      Physics* physics;
      state.get_Physics(bottomId, &physics);
      physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);

      entityId topId = m_pyrTop->getId();

      entityId fireId = m_pyrFire->getId();
      state.add_ScalarMultFunc(fireId, DELEGATE_NOCLASS(pyrFireWiggle));

      //region VIRUS
      Debug::drawLine(state,
                      glm::vec3(10 * delta, 0.f, 4.f),
                      glm::vec3(11 * delta, 0.f, 2.f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(11 * delta, 0.f, 2.f),
                      glm::vec3(12 * delta, 0.f, 4.f),
                      glm::vec3(0.f, 1.f, 0.f));

      Debug::drawLine(state,
                      glm::vec3(13 * delta, 0.f, 2.f),
                      glm::vec3(13 * delta, 0.f, 4.f),
                      glm::vec3(0.f, 1.f, 0.f));

      Debug::drawLine(state,
                      glm::vec3(14 * delta, 0.f, 2.f),
                      glm::vec3(14 * delta, 0.f, 4.f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(14 * delta, 0.f, 4.f),
                      glm::vec3(15 * delta, 0.f, 3.5f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(15 * delta, 0.f, 3.5f),
                      glm::vec3(14 * delta, 0.f, 3.f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(14 * delta, 0.f, 3.f),
                      glm::vec3(15 * delta, 0.f, 2.f),
                      glm::vec3(0.f, 1.f, 0.f));

      Debug::drawLine(state,
                      glm::vec3(16 * delta, 0.f, 2.f),
                      glm::vec3(16 * delta, 0.f, 4.f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(16 * delta, 0.f, 2.f),
                      glm::vec3(17 * delta, 0.f, 2.f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(17 * delta, 0.f, 2.f),
                      glm::vec3(17 * delta, 0.f, 4.f),
                      glm::vec3(0.f, 1.f, 0.f));

      Debug::drawLine(state,
                      glm::vec3(19 * delta, 0.f, 4.f),
                      glm::vec3(18 * delta, 0.f, 3.3f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(18 * delta, 0.f, 3.3f),
                      glm::vec3(19 * delta, 0.f, 2.6f),
                      glm::vec3(0.f, 1.f, 0.f));
      Debug::drawLine(state,
                      glm::vec3(19 * delta, 0.f, 2.6f),
                      glm::vec3(18 * delta, 0.f, 2.f),
                      glm::vec3(0.f, 1.f, 0.f));
      //endregion
      return EZECS_SUCCESS;
    }
    void deInit() {
      physicsSystem.deInit();
    }
    bool systemsHandler(SDL_Event& event) {
      return controlSystem.handleEvent(event);
    }
    void tick(float dt) {
      controlSystem.tick(dt);
      movementSystem.tick(dt);
      physicsSystem.tick(dt);
    }
};

void main_loop(void *instance) {
  PyramidGame *game = (PyramidGame *) instance;
  float dt;
  bool keepGoing = game->mainLoop(game->systemsHandlerDlgt, dt);
  if (!keepGoing) {
    game->deInit();
    exit(0);
  }
  game->tick(dt);
}

int main(int argc, char **argv) {
  PyramidGame game(argc, argv);
  EzecsResult status = game.init();
  if (status.isError()) { fprintf(stderr, "%s", status.toString().c_str()); }

  //region Sound
  Uint8 *gameMusic[4];
  Uint32 gameMusicLength[4];
  int count = SDL_GetNumAudioDevices(0);
  fprintf(stderr, "Number of audio devices: %d\n", count);
  for (int i = 0; i < count; ++i) {
    fprintf(stderr, "Audio device %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
  }
  SDL_AudioSpec want, have;
  SDL_AudioDeviceID dev;
  SDL_memset(&want, 0, sizeof(want));
  want.freq = 4800;
  want.format = AUDIO_F32;
  want.channels = 2;
  want.samples = 4096;
  want.callback = NULL;

  int i = 0;
  SDL_LoadWAV(
      "./assets/audio/TitleScreen.wav",
      &want,
      &gameMusic[i],
      &gameMusicLength[i]);
  i += 1;
  SDL_LoadWAV(
      "./assets/audio/IndustrialTechno_2.wav",
      &want,
      &gameMusic[i],
      &gameMusicLength[i]);
  i += 1;
  SDL_LoadWAV(
      "./assets/audio/YouLose.wav",
      &want,
      &gameMusic[i],
      &gameMusicLength[i]);
  i += 1;
  SDL_LoadWAV(
      "./assets/audio/YouWin.wav",
      &want,
      &gameMusic[i],
      &gameMusicLength[i]);
  i += 1;

  dev = SDL_OpenAudioDevice(
      NULL,  // device
      0,  // is capture
      &want,  // desired
      &have,  // obtained
      0  // allowed changes
  );
  if (dev == 0) {
    fprintf(stderr, "Failed to open SDL audio device: %s\n", SDL_GetError());
  } else {
    for (int i = 0; i < 4; ++i) {
      SDL_QueueAudio(
          dev,
          gameMusic[i],
          gameMusicLength[i]);
    }
    SDL_PauseAudioDevice(dev, 0);  // start audio
  }
  //endregion

  while (true) {
    main_loop(&game);
  }
}
