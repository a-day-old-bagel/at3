
#pragma once

#include <SDL.h>
#include <SDL_hints.h>
#include <string>
#include <topics.hpp>
#include <keyInput.hpp>
#include "debug.h"
#include "config.h"

#define VK_USE_PLATFORM_XCB_KHR

namespace sdl {

  struct WindowCreateInfo {
    std::string title = "Vulkan Binding Benchmark";
    int posX = SDL_WINDOWPOS_UNDEFINED;
    int posY = SDL_WINDOWPOS_UNDEFINED;
    int width = SCREEN_W;
    int height = SCREEN_H;
    uint32_t flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    WindowCreateInfo() = default;
  };

  SDL_Window* init(const WindowCreateInfo &info) {

    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
    int initCode = SDL_Init(SDL_INIT_VIDEO);
    checkf(initCode == 0, "Error initializing SDL: %s\n", SDL_GetError());

    SDL_Window* window = SDL_CreateWindow(
        info.title.c_str(),
        info.posX,
        info.posY,
        info.width,
        info.height,
        info.flags
    );

    checkf(window, "Error creating SDL window: %s\n", SDL_GetError());

    return window;
  }

  void pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_QUIT: {
          rtu::topics::publish("quit");
          return;     // returning here makes sure no further events are handled.
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
            rtu::topics::publish<SDL_Event>("mouse_moved", event);
          } break;
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              rtu::topics::publish("window_resized");
            }
            default: break;
          }
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
  }

}
