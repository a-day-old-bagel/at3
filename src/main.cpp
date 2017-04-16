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
#include <glm/gtc/matrix_transform.hpp>

#include "ezecs.hpp"
#include "debug.h"
#include "game.h"
#include "meshObject.h"
#include "terrainObject.h"
#include "scene.h"
#include "perspectiveCamera.h"
#include "skyBox.h"

#include "ecsSystem_movement.h"
#include "ecsSystem_controls.h"
#include "ecsSystem_physics.h"
#include "ecsSystem_kalman.h"

//#define GET_TIME std::chrono::high_resolution_clock::now();
//#define DURATION std::chrono::duration<double, std::milli>
//#define CHECK(compOpReturn) EZECS_CHECK_PRINT(EZECS_ERR(compOpReturn))

using namespace at3;
using namespace ezecs;

static float pyrFireSize = 0.3f, pyrFireScale = 0.15f;
static glm::mat4 pyrFireWiggle(const glm::mat4& transformIn, uint32_t time) {
  return glm::scale(glm::mat4(), { 1.f, 1.f, pyrFireSize + pyrFireScale * sin(time * 0.1f) });
}

static glm::mat4 pyrTopRotate(const glm::mat4& transformIn, uint32_t time) {
  return glm::rotate(glm::mat4(), time * 0.002f, {0.f, 0.f, 1.f});
}

void debugGenerateFloorGrid(State& state, float xMin, float xMax, unsigned xRes, float yMin, float yMax, unsigned yRes){
  float sclX = (xMax - xMin) * .5f;
  float sclY = (yMax - yMin) * .5f;
  float ctrX = xMin + sclX;
  float ctrY = yMin + sclY;
  float xStep = 2.f / xRes;
  for (unsigned x = 0; x <= xRes; ++x) {
    Debug::drawLine( state,
                     glm::vec3(ctrX + (x * xStep * sclX - sclX), ctrY - sclY, 0.f),
                     glm::vec3(ctrX + (x * xStep * sclX - sclX), ctrY + sclY, 0.f),
                     glm::vec3(1.f, 0.f, 1.f) );
  }
  float yStep = 2.f / yRes;
  for (unsigned y = 0; y <= yRes; ++y) {
    Debug::drawLine( state,
                     glm::vec3(ctrX - sclX, ctrY + (y * yStep * sclY - sclY), 0.f),
                     glm::vec3(ctrX + sclX, ctrY + (y * yStep * sclY - sclY), 0.f),
                     glm::vec3(1.f, 0.f, 1.f) );
  }
}

void debugGenerateVirus(State& state) {
  float delta = 0.3f;
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
}

class PyramidGame : public Game {
  private:
    std::shared_ptr<PerspectiveCamera> m_camera;
    std::shared_ptr<MeshObject> m_pyrBottom, m_pyrTop, m_pyrThrusters, m_pyrFire;
    std::shared_ptr<SceneObject> m_camGimbal;
    std::shared_ptr<SkyBox> m_skyBox;
    std::shared_ptr<TerrainObject> m_terrain;
    std::shared_ptr<BulletDebug> bulletDebug;
    ControlSystem controlSystem;
    MovementSystem movementSystem;
    PhysicsSystem physicsSystem;
//    KalmanSystem kalmanSystem;
    
  public:
    Delegate<bool(SDL_Event&)> systemsHandlerDlgt;
    PyramidGame(int argc, char **argv)
        : Game(argc, argv, "at3"),
          controlSystem(&state), movementSystem(&state), physicsSystem(&state)//, kalmanSystem(&state)
    {
      systemsHandlerDlgt = DELEGATE(&PyramidGame::systemsHandler, this);
    }
    EzecsResult init() {
      bool initSuccess = true;
      initSuccess &= controlSystem.init();
      initSuccess &= physicsSystem.init();
      initSuccess &= movementSystem.init();
//      initSuccess &= kalmanSystem.init();
      assert(initSuccess);

      // generate initial placement of objects
      glm::mat4 ident(1.0); // explicitly identity matrix
      glm::mat4 cameraMat = glm::rotate(glm::translate(ident, {0.f, -4.f, 0.5f}),
                                        (float) M_PI * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
      glm::mat4 pyrBotMat = glm::translate(ident, { 0.f, 0.f, 5.f });
      glm::mat4 pyrFirMat = glm::scale(glm::rotate(glm::translate(ident, {0.f, 0.f, -0.4f}),
                                                   (float) M_PI, glm::vec3(1.0f, 0.0f, 0.0f)), {0.105f, 0.105f, 0.15f});
      glm::mat4 terrainMat = glm::scale(ident, {100.f, 100.f, 100.f});

      // Populate the graphics scene
      m_camera = std::shared_ptr<PerspectiveCamera> (
          new PerspectiveCamera(state, 80.0f * ((float) M_PI / 180.0f), 0.1f, 1000.0f, cameraMat));
      m_pyrBottom = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_bottom.dae", "assets/textures/pyramid_bottom.png", pyrBotMat));
      m_pyrTop = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_top.dae", "assets/textures/pyramid_top_new.png", ident));
      m_pyrThrusters = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_thrusters.dae", "assets/textures/thrusters.png", ident));
      m_pyrFire = std::shared_ptr<MeshObject> (
          new MeshObject(state, "assets/models/pyramid_thruster_flames.dae", "assets/textures/pyramid_flames.png",
                         pyrFirMat));
      m_camGimbal = std::shared_ptr<SceneObject> (
          new SceneObject(state));
      m_skyBox = std::shared_ptr<SkyBox> (
          new SkyBox(state));
      m_terrain = std::shared_ptr<TerrainObject> (
          new TerrainObject(state, terrainMat, 0, 0, 0, 0));

      this->scene()->addObject(m_pyrBottom);
      this->scene()->addObject(m_skyBox);
      this->scene()->addObject(m_terrain);
      LoadResult loaded = m_skyBox->useCubeMap("nalovardo", "png");
      assert(loaded == LOAD_SUCCESS);

      m_pyrBottom->addChild(m_camGimbal);
      m_pyrBottom->addChild(m_pyrTop);
      m_pyrBottom->addChild(m_pyrThrusters);
      m_pyrBottom->addChild(m_pyrFire);

      m_camGimbal->addChild(m_camera);
      this->setCamera(m_camera);

      entityId gimbalId = m_camGimbal->getId();
      state.add_Placement(gimbalId, {
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, 0, 1, 1  // at (0, 0, 1)
      });
      state.add_MouseControls(gimbalId, false, false);

      entityId bottomId = m_pyrBottom->getId();
      state.add_PyramidControls(bottomId, gimbalId, PyramidControls::ROTATE_ABOUT_Z);
      std::vector<float> hullVerts = {
          1.0f,  1.0f, -0.4f,
          1.0f, -1.0f, -0.4f,
          0.f ,  0.f ,  1.7f,
          1.0f,  1.0f, -0.4f,
         -1.0f,  1.0f, -0.4f,
          0.f ,  0.f ,  1.7f,
         -1.0f, -1.0f, -0.4f,
         -1.0f,  1.0f, -0.4f,
          1.0f, -1.0f, -0.4f,
         -1.0f, -1.0f, -0.4f,
      };
      state.add_Physics(bottomId, 1.f, &hullVerts, Physics::MESH);

      Physics* physics;
      state.get_Physics(bottomId, &physics);
      physics->rigidBody->setActivationState(DISABLE_DEACTIVATION);
      physics->rigidBody->setDamping(0.1f, 0.8f);

      entityId topId = m_pyrTop->getId();
      state.add_TransformFunction(topId, DELEGATE_NOCLASS(pyrTopRotate));

      entityId fireId = m_pyrFire->getId();
      state.add_TransformFunction(fireId, DELEGATE_NOCLASS(pyrFireWiggle));
      
      entityId botId = m_pyrBottom->getId();
      state.add_Kalman(botId);


      // Add some debug-drawn features...

      // a floor grid
      debugGenerateFloorGrid(state, -10.5f, 10.5f, 21, -10.5f, 10.5f, 21);
      // a virus
      debugGenerateVirus(state);
      // a bullet physics debug-drawing thing
      /*bulletDebug = std::shared_ptr<BulletDebug> ( new BulletDebug(&state) );
      bulletDebug->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
      physicsSystem.activateDebugDrawer(bulletDebug);
      this->scene()->addObject(bulletDebug);*/

      return EZECS_SUCCESS;
    }
    void deInit() {
      physicsSystem.deInit();
    }
    bool systemsHandler(SDL_Event& event) {
      if (controlSystem.handleEvent(event)) {
        return true;
      }
//      if (kalmanSystem.handleEvent(event)) {
//        return true;
//      }
      return false;
    }
    void tick(float dt) {
      controlSystem.tick(dt);
      physicsSystem.tick(dt);
      movementSystem.tick(dt);
//      kalmanSystem.tick(dt);

      // make the fire look big if the pyramid is thrusting upwards
      PyramidControls* controls;
      state.get_PyramidControls(m_pyrBottom->getId(), &controls);
      if (controls->accel.z > 0) {
        pyrFireSize = 1.5f;
        pyrFireScale = 1.f;
      } else {
        pyrFireSize = 0.3f;
        pyrFireScale = 0.15f;
      }
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

/*  //region Sound
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
  //endregion*/

  while (true) {
    main_loop(&game);
  }
}
