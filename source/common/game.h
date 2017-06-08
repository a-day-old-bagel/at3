
#pragma once

#define TIME_MULTIPLIER_MS 0.001f

#include <memory>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "graphicsBackend.h"
#include "debug.h"
#include "scene.h"
#include "game.h"

namespace at3 {

  typedef std::function<bool(SDL_Event &)> eventHandlerFunction;

  template <typename EcsInterface> class Camera;
  template <typename EcsInterface> class Scene;

  template <typename EcsState, typename EcsInterface>
  class Game {

    private:
      std::shared_ptr<Camera<EcsInterface>> mpCamera;
      float mLastTime = 0.f;
      std::string settingsFileName;

    protected:
      EcsState mState;
      Scene<EcsInterface> mScene;

      virtual void registerCustomSettings() { }

    public:
      Game(int argc, char **argv, const char *appName = "at3", const char *settingsName = "settings.ini");
      virtual ~Game();

      void setCamera(std::shared_ptr<Camera<EcsInterface>> camera);
      std::shared_ptr<Camera<EcsInterface>> getCamera() { return mpCamera; }

      virtual bool handleEvent(const SDL_Event &event) {
        return false;
      }

      bool mainLoop(eventHandlerFunction &systemsHandler, float &dtOut);
  };

  template <typename EcsState, typename EcsInterface>
  Game<EcsState, EcsInterface>::Game(int argc, char **argv, const char *appName, const char *settingsName) {
    settingsFileName = settingsName;
    graphicsBackend::applicationName = appName;
    registerCustomSettings();
    settings::loadFromIni(settingsFileName.c_str());
    if ( ! graphicsBackend::init() ) {
      std::cerr << "Could not initialize graphics backend!" << std::endl;
      exit(-2);
    }
  }

  template <typename EcsState, typename EcsInterface>
  Game<EcsState, EcsInterface>::~Game() {

  }

  template <typename EcsState, typename EcsInterface>
  bool Game<EcsState, EcsInterface>::mainLoop(eventHandlerFunction &systemsHandler, float &dtOut) {

    graphicsBackend::swap();
    if (mLastTime == 0.0f) {
      // FIXME: Try to make sure this doesn't ever produce a dt of 0.
      mLastTime = (float)(std::min((Uint32)0, SDL_GetTicks() - 1)) * TIME_MULTIPLIER_MS;
    }

    // poll events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        settings::saveToIni(settingsFileName.c_str());
        return false;
      }
      if (systemsHandler(event)) { continue; }                // The systems have handled this event
      if (graphicsBackend::handleEvent(event)) { continue; }  // Graphics backend handled it
      if (this->handleEvent(event)) { continue; }             // Our derived class handled this event
      if (mScene.handleEvent(event)) { continue; }            // One of the scene objects handled this event
    }

    // Calculate the time since the last frame was drawn TODO: SDL_GetTicks may be too granular
    float currentTime = (float)SDL_GetTicks() * TIME_MULTIPLIER_MS;
    float dt = currentTime - mLastTime;
    mLastTime = currentTime;

    graphicsBackend::clear();

    if (!mpCamera) {
      fprintf(stderr, "The camera is not set!\n");
    } else {
      mScene.draw(*mpCamera, graphicsBackend::getAspect());
    }

    dtOut = dt;
    return true;
  }

  template <typename EcsState, typename EcsInterface>
  void Game<EcsState, EcsInterface>::setCamera(std::shared_ptr<Camera<EcsInterface>> camera) {
    mpCamera = camera;
    graphicsBackend::setFovy(camera->getFovy());
  }
}
