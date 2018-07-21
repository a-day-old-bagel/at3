
#pragma once

#if !SDL_VERSION_ATLEAST(2, 0, 8)
#error "SDL2 version 2.0.8 or newer required."
#endif

#include <memory>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "sdlc.hpp"
#include "vkc.hpp"
#include "sceneTree.hpp"
#include "netInterface.hpp"
#include "keyInput.hpp"

namespace at3 {

  template <typename EcsInterface> class ObjCamera;
  template <typename EcsInterface> class SceneTree;

  template <typename EcsInterface, typename Derived>
  class Game {

    public:

      Game();
      virtual ~Game() = default;
      bool init(const char *appName, const char *settingsName);
      void tick();
      bool getIsQuit();

    protected:

      typename EcsInterface::State state;
      SceneTree<EcsInterface> scene;

      std::unique_ptr<SdlContext> sdlc;
      std::unique_ptr<vkc::VulkanContext<EcsInterface>> vulkan;
      std::shared_ptr<NetInterface> network;

    private:

      std::unique_ptr<EcsInterface> ecsInterface;
      std::shared_ptr<ObjCamera<EcsInterface>> currentCamera = nullptr;
      rtu::topics::Subscription switchToCamSub;
      float lastTime = 0.f;
      std::string settingsFileName;
      bool isQuit = false;

      Derived &topLevel();
      void setCamera(void *camPtr);
      void deInit();

  };

  template<typename EcsInterface, typename Derived>
  Game<EcsInterface, Derived>::Game() : switchToCamSub("set_primary_camera", RTU_MTHD_DLGT(&Game::setCamera, this)) { }

  template <typename EcsInterface, typename Derived>
  Derived & Game<EcsInterface, Derived>::topLevel() {
    return *static_cast<Derived*>(this);
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::init(const char *appName, const char *settingsName) {

    ecsInterface = std::make_unique<EcsInterface>(&state);
    Object::linkEcs(*ecsInterface);

    settingsFileName = settingsName;
    topLevel().registerCustomSettings();
    settings::loadFromIni(settingsFileName.c_str());

    sdlc = std::make_unique<SdlContext>(appName);

    vkc::VulkanContextCreateInfo<EcsInterface> contextCreateInfo =
        vkc::VulkanContextCreateInfo<EcsInterface>::defaults();
    contextCreateInfo.window = sdlc->getWindow();
    contextCreateInfo.ecs = ecsInterface.get();
    vulkan = std::make_unique<vkc::VulkanContext<EcsInterface>>(contextCreateInfo);

    network = std::make_shared<NetInterface>();
    rtu::topics::publish<std::shared_ptr<NetInterface> &>("set_network_interface", network);

    return topLevel().onInit();
  }

  template <typename EcsInterface, typename Derived>
  bool Game<EcsInterface, Derived>::getIsQuit() {
    return isQuit;
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::deInit() {
    isQuit = true;
    currentCamera.reset();
    settings::saveToIni(settingsFileName.c_str());
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::tick() {

    // If this is first frame, make sure timing doesn't cause problems
    if (lastTime == 0.0f) {
      // FIXME: Try to make sure this doesn't ever produce a dt of 0.
      // TODO: Actually, just use the std::chrono stuff and use its "now" function
      lastTime = (float)(std::min((Uint32)0, SDL_GetTicks() - 1)) * TIME_MULTIPLIER_MS;
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
              SDL_Window *window = sdlc->getWindow();
              if(SDL_GetWindowGrab(window)) {  // window is currently grabbed - ungrab.
                SDL_SetWindowGrab(window, SDL_FALSE);
                SDL_SetRelativeMouseMode(SDL_FALSE);
              } else {  // window is not currently grabbed - grab.
                SDL_SetWindowGrab(window, SDL_TRUE);
                SDL_SetRelativeMouseMode(SDL_TRUE);
              }
            } break;

              // SOME FUNCTION KEYS ARE RESERVED: They do not emit a key down signal, but rather
              // the desired effect directly.
            // TODO: map F1 to new, functional, fullscreen-toggling code (copy from graphicsBackend.cpp to start)
            // TODO: map F2 to new, functional, debug-draw-toggling code (this used to do terrain edge view)
            case SDL_SCANCODE_F1: rtu::topics::publish("key_down_f1"); break;
            case SDL_SCANCODE_F2: rtu::topics::publish("key_down_f2"); break;
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
            case SDL_SCANCODE_R: rtu::topics::publish("key_down_r"); break;
            case SDL_SCANCODE_SPACE: rtu::topics::publish("key_down_space"); break;
            default: break;
          } break;
        case SDL_MOUSEBUTTONDOWN:
          switch(event.button.button) {
            case SDL_BUTTON_LEFT: rtu::topics::publish("mouse_down_left"); break;
            case SDL_BUTTON_RIGHT: rtu::topics::publish("mouse_down_right"); break;
            case SDL_BUTTON_MIDDLE: rtu::topics::publish("mouse_down_middle"); break;
            case SDL_BUTTON_X1: rtu::topics::publish("mouse_down_x1"); break;
            case SDL_BUTTON_X2: rtu::topics::publish("mouse_down_x2"); break;
            default: break;
          } break;
        case SDL_MOUSEMOTION:
          if (SDL_GetRelativeMouseMode()) {
            rtu::topics::publish<SDL_Event>("mouse_moved", event);
          } break;
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              settings::graphics::windowDimX = (uint32_t) event.window.data1;
              settings::graphics::windowDimY = (uint32_t) event.window.data2;
              vulkan->reInitRendering();
              std::cout << "Window size changed to: " << settings::graphics::windowDimX << "x"
                        << settings::graphics::windowDimY << std::endl;
            } break;
            case SDL_WINDOWEVENT_MOVED: {
              settings::graphics::windowPosX = event.window.data1;
              settings::graphics::windowPosY = event.window.data2;
              printf("SDL_WINDOWEVENT_MOVED\n");
            } break;
            case SDL_WINDOWEVENT_MAXIMIZED: {
              settings::graphics::fullscreen = settings::graphics::MAXIMIZED;
              printf("SDL_WINDOWEVENT_MAXIMIZED\n");
            } break;
            case SDL_WINDOWEVENT_RESTORED: {
              settings::graphics::fullscreen = settings::graphics::WINDOWED;
              printf("SDL_WINDOWEVENT_RESTORED\n");
            } break;
#               if AT3_DEBUG_WINDOW_EVENTS
            case SDL_WINDOWEVENT_SHOWN: {
                  printf("SDL_WINDOWEVENT_SHOWN\n");
                } break;
                case SDL_WINDOWEVENT_HIDDEN: {
                  printf("SDL_WINDOWEVENT_HIDDEN\n");
                } break;
                case SDL_WINDOWEVENT_EXPOSED: {
                  printf("SDL_WINDOWEVENT_EXPOSED\n");
                } break;
                case SDL_WINDOWEVENT_RESIZED: {
                  printf("SDL_WINDOWEVENT_RESIZED\n");
                } break;
                case SDL_WINDOWEVENT_MINIMIZED: {
                  printf("SDL_WINDOWEVENT_MINIMIZED\n");
                } break;
                case SDL_WINDOWEVENT_ENTER: {
                  printf("SDL_WINDOWEVENT_ENTER\n");
                } break;
                case SDL_WINDOWEVENT_LEAVE: {
                  printf("SDL_WINDOWEVENT_LEAVE\n");
                } break;
                case SDL_WINDOWEVENT_FOCUS_GAINED: {
                  printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
                } break;
                case SDL_WINDOWEVENT_FOCUS_LOST: {
                  printf("SDL_WINDOWEVENT_FOCUS_LOST\n");
                } break;
                case SDL_WINDOWEVENT_CLOSE: {
                  printf("SDL_WINDOWEVENT_CLOSE\n");
                } break;
                case SDL_WINDOWEVENT_TAKE_FOCUS: {
                  printf("SDL_WINDOWEVENT_TAKE_FOCUS\n");
                } break;
                case SDL_WINDOWEVENT_HIT_TEST: {
                  printf("SDL_WINDOWEVENT_HIT_TEST\n");
                } break;
#               endif
            default: break;
          } break;
        default: break;
      }
    }

    // publish current keyboard state
    const Uint8 *keyStates = SDL_GetKeyboardState(NULL);
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

    // publish current mouse state
    const uint32_t mouseState = SDL_GetMouseState(nullptr, nullptr);
    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) rtu::topics::publish("mouse_held_left");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) rtu::topics::publish("mouse_held_right");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) rtu::topics::publish("mouse_held_middle");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_X1)) rtu::topics::publish("mouse_held_x1");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_X2)) rtu::topics::publish("mouse_held_x2");

    // Call any network updates that need to happen
    network->tick();

    // Update logic given the time since the last frame was drawn TODO: SDL_GetTicks may be too granular
    float currentTime = (float)SDL_GetTicks() * TIME_MULTIPLIER_MS;
    float dt = currentTime - lastTime;
    lastTime = currentTime;
    topLevel().onTick(dt);

    if (currentCamera) {
      scene.updateAbsoluteTransformCaches();
      rtu::topics::publish<glm::mat4>("primary_cam_wv", currentCamera->worldView());
      vulkan->step();
    }
  }

  template <typename EcsInterface, typename Derived>
  void Game<EcsInterface, Derived>::setCamera(void *camPtr) {
    std::shared_ptr<ObjCamera<EcsInterface>> *camera = (std::shared_ptr<ObjCamera<EcsInterface>>*)camPtr;
    currentCamera = *camera;
  }
}
