
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

  template <typename EcsInterface> class Camera;
  template <typename EcsInterface> class Scene;

  template <typename EcsState, typename EcsInterface, typename Derived>
  class Game {

    private:
      std::shared_ptr<Camera<EcsInterface>> mpCamera;
      float mLastTime = 0.f;
      std::string mSettingsFileName;
      bool mIsQuit = false;

      Derived &derived();

    protected:
      EcsState mState;
      Scene<EcsInterface> mScene;

    public:

      bool init(const char *appName, const char *settingsName);
      void tick();
      bool isQuit();
      void deInit();

      void setCamera(std::shared_ptr<Camera<EcsInterface>> camera);
      std::shared_ptr<Camera<EcsInterface>> getCamera() { return mpCamera; }
  };

  template <typename EcsState, typename EcsInterface, typename Derived>
  Derived & Game<EcsState, EcsInterface, Derived>::derived() {
    return *static_cast<Derived*>(this);
  }

  template <typename EcsState, typename EcsInterface, typename Derived>
  bool Game<EcsState, EcsInterface, Derived>::init(const char *appName, const char *settingsName) {
    mSettingsFileName = settingsName;
    graphicsBackend::applicationName = appName;
    derived().registerCustomSettings();
    settings::loadFromIni(mSettingsFileName.c_str());
    if (!graphicsBackend::init()) { return false; }
    if (!derived().onInit()) { return false; }
    return true;
  }

  template <typename EcsState, typename EcsInterface, typename Derived>
  bool Game<EcsState, EcsInterface, Derived>::isQuit() {
    return mIsQuit;
  }

  template <typename EcsState, typename EcsInterface, typename Derived>
  void Game<EcsState, EcsInterface, Derived>::deInit() {
    mIsQuit = true;
    derived().onDeInit();
    graphicsBackend::deinit();
    settings::saveToIni(mSettingsFileName.c_str());
  }

  template <typename EcsState, typename EcsInterface, typename Derived>
  void Game<EcsState, EcsInterface, Derived>::tick() {

    // Previous draw now finished, put it on screen
    graphicsBackend::swap();

    // If this is first frame, make sure timing doesn't cause problems
    if (mLastTime == 0.0f) {
      // FIXME: Try to make sure this doesn't ever produce a dt of 0.
      mLastTime = (float)(std::min((Uint32)0, SDL_GetTicks() - 1)) * TIME_MULTIPLIER_MS;
    }

    // Poll events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) { deInit(); return; }
      if (graphicsBackend::handleEvent(event)) { continue; }  // Graphics backend handled it exclusively
      if (derived().handleEvent(event)) { continue; }             // The derived object handled it exclusively
      if (mScene.handleEvent(event)) { continue; }            // One of the scene objects handled it exclusively
    }

    // Update logic given the time since the last frame was drawn TODO: SDL_GetTicks may be too granular
    float currentTime = (float)SDL_GetTicks() * TIME_MULTIPLIER_MS;
    float dt = currentTime - mLastTime;
    mLastTime = currentTime;
    derived().onTick(dt);

    // Clear the graphics scene and begin redraw
    graphicsBackend::clear();
    if (!mpCamera) {
      fprintf(stderr, "The camera is not set!\n");
    } else {
      mScene.draw(*mpCamera, graphicsBackend::getAspect());
    }

  }

  template <typename EcsState, typename EcsInterface, typename Derived>
  void Game<EcsState, EcsInterface, Derived>::setCamera(std::shared_ptr<Camera<EcsInterface>> camera) {
    mpCamera = camera;
    graphicsBackend::setFovy(camera->getFovy());
  }
}
