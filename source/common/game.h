
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

namespace at3 {

  template <typename EcsInterface> class Camera;
  template <typename EcsInterface> class Scene;

  template <typename EcsInterface, typename Derived>
  class Game {

    protected:
      typename EcsInterface::State mState;
      Scene<EcsInterface> mScene;

    private:
      EcsInterface mEcsInterface;
      std::shared_ptr<Camera<EcsInterface>> mpCamera;
      float mLastTime = 0.f;
      std::string mSettingsFileName;
      bool mIsQuit = false;

      Derived &derived();

    public:

      Game() : mEcsInterface(&mState) { SceneObject_::linkEcs(mEcsInterface); }
      virtual ~Game() { std::cout << "Game is destructing." << std::endl; };

      bool init(const char *appName, const char *settingsName);
      void tick();
      bool isQuit();
      void deInit();

      void setCamera(std::shared_ptr<Camera<EcsInterface>> camera);
      std::shared_ptr<Camera<EcsInterface>> getCamera() { return mpCamera; }
  };

  template <typename EcsInterface, typename Derived>
  Derived & Game<EcsInterface, Derived>::derived() {
    return *static_cast<Derived*>(this);
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::init(const char *appName, const char *settingsName) {
    mSettingsFileName = settingsName;
    graphicsBackend::applicationName = appName;
    derived().registerCustomSettings();
    settings::loadFromIni(mSettingsFileName.c_str());
    if (!graphicsBackend::init()) { return false; }
    if (!derived().onInit()) { return false; }
    return true;
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::isQuit() {
    return mIsQuit;
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::deInit() {
    mIsQuit = true;
    mpCamera.reset();
    graphicsBackend::deInit();
    settings::saveToIni(mSettingsFileName.c_str());
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::tick() {

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
      if (event.type == SDL_QUIT) {
        deInit();   // May deallocate some important things
        return;     // So return makes sure no more events are processed
      }
      if (graphicsBackend::handleEvent(event)) { continue; }  // Graphics backend handled it exclusively
      if (derived().handleEvent(event)) { continue; }         // The derived object handled it exclusively
      if (mScene.handleEvent(event)) { continue; }            // One of the scene objects handled it exclusively
    }

    // Update logic given the time since the last frame was drawn TODO: SDL_GetTicks may be too granular
    float currentTime = (float)SDL_GetTicks() * TIME_MULTIPLIER_MS;
    float dt = currentTime - mLastTime;
    mLastTime = currentTime;
    derived().onTick(dt);

    // Clear the graphics scene and begin redraw if a camera is assigned
    graphicsBackend::clear();
    if (mpCamera) {
      mScene.draw(*mpCamera, false);
    }
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::setCamera(std::shared_ptr<Camera<EcsInterface>> camera) {
    mpCamera = camera;
    graphicsBackend::setFovy(camera->getFovy());
  }
}
