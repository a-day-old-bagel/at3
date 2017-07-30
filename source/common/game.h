
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
#include "keyInput.hpp"

namespace at3 {

//  template<typename lastKeyCode>
//  static bool anyPressed(const Uint8 *keyStates, lastKeyCode key) {
//    return keyStates[key];
//  }
//
//  template<typename firstKeyCode, typename... keyCode>
//  static bool anyPressed(const Uint8 *keyStates, firstKeyCode firstKey, keyCode... keys) {
//    return keyStates[firstKey] || anyPressed(keyStates, keys...);
//  }

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
      rtu::topics::Subscription switchToCamSub;

      Derived &derived();

    public:

      Game();
//      Game() : mEcsInterface(&mState),
//        switchToCamSub("set_primary_camera", rtu::NewDelegate(&Game<EcsInterface,
//            Derived>::setCamera).Create<Game<EcsInterface, Derived>::setCamera>(this))
//      { SceneObject_::linkEcs(mEcsInterface); }
      virtual ~Game() { std::cout << "Game is destructing." << std::endl; };

      bool init(const char *appName, const char *settingsName);
      void tick();
      bool isQuit();
      void deInit();

//      void setCamera(std::shared_ptr<Camera<EcsInterface>> camera);
      void setCamera(void *camPtr);
      std::shared_ptr<Camera<EcsInterface>> getCamera() { return mpCamera; }
  };

  template <typename EcsInterface, typename Derived>
  Game<EcsInterface, Derived>::Game() : mEcsInterface(&mState),
      switchToCamSub("set_primary_camera", rtu::NewDelegate(&Game<EcsInterface,
           Derived>::setCamera).Create<&Game<EcsInterface, Derived>::setCamera>(this))
  { SceneObject_::linkEcs(mEcsInterface); }

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

    // Update the world-view matrix topic
    rtu::topics::publish("primary_cam_wv", (void*)&mpCamera->lastWorldViewQueried);

    // Poll SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_QUIT: {
          deInit();   // This may deallocate some important things that are used in event handling.
          return;     // Therefore returning here makes sure no further events are handled.
        }
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE: {
              SDL_Window *window = SDL_GetGrabbedWindow();
              if (window == nullptr)
                break;
              // Release the mouse cursor from the window on escape pressed
              SDL_SetWindowGrab(window, SDL_FALSE);
#             if !SDL_VERSION_ATLEAST(2, 0, 4)
                grabbedWindow = nullptr;
#             endif
              SDL_SetRelativeMouseMode(SDL_FALSE);
            } break;
            case SDL_SCANCODE_0: rtu::topics::publish("key_down_0", nullptr); break;
            case SDL_SCANCODE_1: rtu::topics::publish("key_down_1", nullptr); break;
            case SDL_SCANCODE_2: rtu::topics::publish("key_down_2", nullptr); break;
            case SDL_SCANCODE_3: rtu::topics::publish("key_down_3", nullptr); break;
            case SDL_SCANCODE_4: rtu::topics::publish("key_down_4", nullptr); break;
            case SDL_SCANCODE_5: rtu::topics::publish("key_down_5", nullptr); break;
            case SDL_SCANCODE_6: rtu::topics::publish("key_down_6", nullptr); break;
            case SDL_SCANCODE_7: rtu::topics::publish("key_down_7", nullptr); break;
            case SDL_SCANCODE_8: rtu::topics::publish("key_down_8", nullptr); break;
            case SDL_SCANCODE_9: rtu::topics::publish("key_down_9", nullptr); break;
            case SDL_SCANCODE_F: rtu::topics::publish("key_down_f", nullptr); break;
            case SDL_SCANCODE_SPACE: rtu::topics::publish("key_down_space", nullptr); break;
            default: break;
          } break;
        default: break;
      }
      if (graphicsBackend::handleEvent(event)) { continue; }  // Graphics backend handled it exclusively
      if (derived().handleEvent(event)) { continue; }         // The derived object handled it exclusively
      if (mScene.handleEvent(event)) { continue; }            // One of the scene objects handled it exclusively
    }

    // Get current keyboard state
    const Uint8 *keyStates = SDL_GetKeyboardState(NULL);
    // Publish to "key_held" topics
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_w", nullptr), keyStates, SDL_SCANCODE_W)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_s", nullptr), keyStates, SDL_SCANCODE_S)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_d", nullptr), keyStates, SDL_SCANCODE_D)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_a", nullptr), keyStates, SDL_SCANCODE_A)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_e", nullptr), keyStates, SDL_SCANCODE_E)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_q", nullptr), keyStates, SDL_SCANCODE_Q)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_f", nullptr), keyStates, SDL_SCANCODE_F)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_space", nullptr), keyStates, SDL_SCANCODE_SPACE)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_lshift", nullptr), keyStates, SDL_SCANCODE_LSHIFT)

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
  void Game<EcsInterface, Derived>::setCamera(void *camPtr) {
    std::shared_ptr<Camera<EcsInterface>> *camera = (std::shared_ptr<Camera<EcsInterface>>*)camPtr;
    mpCamera = *camera;
    graphicsBackend::setFovy(mpCamera->getFovy());
  }

//  template <typename EcsInterface, typename Derived>
//  void Game<EcsInterface, Derived>::setCamera(std::shared_ptr<Camera<EcsInterface>> camera) {
//    mpCamera = camera;
//    graphicsBackend::setFovy(camera->getFovy());
//  }
}
