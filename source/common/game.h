
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

  template <typename EcsInterface> class Camera;
  template <typename EcsInterface> class Scene;

  template <typename EcsInterface, typename Derived>
  class Game {

    protected:
      typename EcsInterface::State mState;
      Scene<EcsInterface> mScene;

    private:
      EcsInterface mEcsInterface;
      std::shared_ptr<Camera<EcsInterface>> mpCamera = nullptr;
      float mLastTime = 0.f;
      std::string mSettingsFileName;
      bool mIsQuit = false;
      rtu::topics::Subscription switchToCamSub;

      Derived &derived();

    public:

      Game();
      virtual ~Game() { std::cout << "Game is destructing." << std::endl; };

      bool init(const char *appName, const char *settingsName);
      void tick();
      bool isQuit();
      void deInit();

      void setCamera(void *camPtr);
      std::shared_ptr<Camera<EcsInterface>> getCamera() { return mpCamera; }
  };

  #define GAME_TYPE typename Game<EcsInterface, Derived>

  template <typename EcsInterface, typename Derived>
  using GameType = Game<EcsInterface, Derived>;

  template <typename EcsInterface, typename Derived>
  Game<EcsInterface, Derived>::Game() : mEcsInterface(&mState),
          switchToCamSub("set_primary_camera", RTU_MTHD_DLGT((&Game<EcsInterface, Derived>::setCamera), this))
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
    return derived().onInit();
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
      // TODO: Actually, just use the std::chrono stuff and use its "now" function
      mLastTime = (float)(std::min((Uint32)0, SDL_GetTicks() - 1)) * TIME_MULTIPLIER_MS;
    }

    // Update the world-view matrix topic
    if (mpCamera) {
//      rtu::topics::publish("primary_cam_wv", (void *) &mpCamera->lastWorldViewQueried);
      rtu::topics::publish<glm::mat4>("primary_cam_wv", mpCamera->lastWorldViewQueried);
    }

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

              // SOME FUNCTION KEYS ARE RESERVED: They do not emit a key down signal, but rather
              // the desired effect directly.
            case SDL_SCANCODE_F1: graphicsBackend::toggleFullscreen(nullptr); break;
            case SDL_SCANCODE_F2: Shaders::toggleEdgeView(nullptr); break;
            case SDL_SCANCODE_F3: rtu::topics::publish("key_down_f3"); break;
            case SDL_SCANCODE_F4: rtu::topics::publish("key_down_f4"); break;
            case SDL_SCANCODE_F5: rtu::topics::publish("key_down_f5"); break;
            case SDL_SCANCODE_F6: rtu::topics::publish("key_down_f6"); break;
            case SDL_SCANCODE_F7: rtu::topics::publish("key_down_f7"); break;
            case SDL_SCANCODE_F8: rtu::topics::publish("key_down_f8"); break;
            case SDL_SCANCODE_F9: rtu::topics::publish("key_down_f9"); break;
            case SDL_SCANCODE_F10: rtu::topics::publish("key_down_f10"); break;
            case SDL_SCANCODE_F11: rtu::topics::publish("key_down_f11"); break;
            case SDL_SCANCODE_F12: rtu::topics::publish("key_down_f12"); break;

            case SDL_SCANCODE_0: rtu::topics::publish("key_down_0"); break;
            case SDL_SCANCODE_1: rtu::topics::publish("key_down_1"); break;
            case SDL_SCANCODE_2: rtu::topics::publish("key_down_2"); break;
            case SDL_SCANCODE_3: rtu::topics::publish("key_down_3"); break;
            case SDL_SCANCODE_4: rtu::topics::publish("key_down_4"); break;
            case SDL_SCANCODE_5: rtu::topics::publish("key_down_5"); break;
            case SDL_SCANCODE_6: rtu::topics::publish("key_down_6"); break;
            case SDL_SCANCODE_7: rtu::topics::publish("key_down_7"); break;
            case SDL_SCANCODE_8: rtu::topics::publish("key_down_8"); break;
            case SDL_SCANCODE_9: rtu::topics::publish("key_down_9"); break;
            case SDL_SCANCODE_F: rtu::topics::publish("key_down_f"); break;
            case SDL_SCANCODE_SPACE: rtu::topics::publish("key_down_space"); break;
            default: break;
          } break;
        case SDL_MOUSEBUTTONDOWN:
          // Make sure the window has grabbed the mouse cursor
          if (SDL_GetGrabbedWindow() == nullptr) {
            // Somehow obtain a pointer for the window
            SDL_Window *window = SDL_GetWindowFromID(event.button.windowID);
            if (window == nullptr)
              break;
            // Grab the mouse cursor
            SDL_SetWindowGrab(window, SDL_TRUE);
#           if (!SDL_VERSION_ATLEAST(2, 0, 4))
              grabbedWindow = window;
#           endif
            SDL_SetRelativeMouseMode(SDL_TRUE);
          } break;
        case SDL_MOUSEMOTION:
          if (SDL_GetRelativeMouseMode()) {
//            rtu::topics::publish("mouse_moved", (void *) &event);
            rtu::topics::publish<SDL_Event>("mouse_moved", event);
          } break;
        case SDL_WINDOWEVENT:
          graphicsBackend::handleWindowEvent((void*)&event); break;
        default: break;
      }
    }

    // Get current keyboard state
    const Uint8 *keyStates = SDL_GetKeyboardState(NULL);
    // Publish to "key_held" topics
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_w"), keyStates, SDL_SCANCODE_W)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_s"), keyStates, SDL_SCANCODE_S)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_d"), keyStates, SDL_SCANCODE_D)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_a"), keyStates, SDL_SCANCODE_A)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_e"), keyStates, SDL_SCANCODE_E)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_q"), keyStates, SDL_SCANCODE_Q)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_f"), keyStates, SDL_SCANCODE_F)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_c"), keyStates, SDL_SCANCODE_C)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_space"), keyStates, SDL_SCANCODE_SPACE)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_lshift"), keyStates, SDL_SCANCODE_LSHIFT)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_lalt"), keyStates, SDL_SCANCODE_LALT)
    RTU_DO_ON_KEYS(rtu::topics::publish("key_held_lctrl"), keyStates, SDL_SCANCODE_LCTRL)

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
}
