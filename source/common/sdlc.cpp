
#include "sdlc.hpp"
#include "settings.hpp"
#include "topics.hpp"
#include "keyInput.hpp"

using namespace rtu::topics;

namespace at3 {

  SdlContext::SdlContext(const char *applicationName) {
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
      fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    }
    switch (settings::graphics::fullscreen) {
      case settings::graphics::FAKED_FULLSCREEN: {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
      } break;
      case settings::graphics::FULLSCREEN: {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
      } break;
      case settings::graphics::WINDOWED:
      default: {
        windowFlags |= SDL_WINDOW_RESIZABLE;
      } break;
    }
    window = SDL_CreateWindow(
        applicationName,                // window title
        settings::graphics::windowPosX, // x
        settings::graphics::windowPosY, // y
        settings::graphics::windowDimX, // width
        settings::graphics::windowDimY, // height
        windowFlags                     // flags
    );
    if (window == nullptr) {
      fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
    }
    lastTime = SDL_GetPerformanceCounter();
    toSeconds = 1.0 / (double)SDL_GetPerformanceFrequency();
  }

  SDL_Window * SdlContext::getWindow() {
    return window;
  }

  bool SdlContext::setFullscreenMode(uint32_t mode) {
    switch (mode) {
      case settings::graphics::FULLSCREEN: {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
      } break;
      case settings::graphics::FAKED_FULLSCREEN: {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
      } break;
      case settings::graphics::MAXIMIZED: SDL_MaximizeWindow(window);
      case settings::graphics::WINDOWED:
      default: {
        SDL_SetWindowFullscreen(window, 0);
      }
    }
    auto isCurrentlyFull = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
    auto isCurrentlyFake = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (isCurrentlyFull) {
      settings::graphics::fullscreen = settings::graphics::FULLSCREEN;
    } else if (isCurrentlyFake) {
      settings::graphics::fullscreen = settings::graphics::FAKED_FULLSCREEN;
    } else {
      settings::graphics::fullscreen = settings::graphics::WINDOWED;
    }
    return (settings::graphics::fullscreen == mode);
  }

  void SdlContext::toggleFullscreen() {
    auto isCurrentlyFull = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
    auto isCurrentlyFake = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (isCurrentlyFull || isCurrentlyFake) {
      if ( ! setFullscreenMode(settings::graphics::WINDOWED)) {
        fprintf(stderr, "Failed to change to windowed mode!\n");
      }
    } else {
      if ( ! setFullscreenMode(settings::graphics::FAKED_FULLSCREEN)) {
        fprintf(stderr, "Failed to change to windowed fullscreen mode!\n");
      }
    }
  }

  void SdlContext::publishEvents() {
    // Poll SDL events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch(event.type) {
        case SDL_QUIT: {
          publish("quit"); // This may deallocate some important things that are used in event handling.
          return;          // Therefore returning here makes sure no further events are handled.
        }
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE: {
              if(SDL_GetWindowGrab(window)) {  // window is currently grabbed - ungrab.
                SDL_SetWindowGrab(window, SDL_FALSE);
                SDL_SetRelativeMouseMode(SDL_FALSE);
              } else {  // window is not currently grabbed - grab.
                SDL_SetWindowGrab(window, SDL_TRUE);
                SDL_SetRelativeMouseMode(SDL_TRUE);
              }
            } break;

            // TODO: map F1 to new, functional, fullscreen-toggling code (copy from graphicsBackend.cpp to start)
            // TODO: map F2 to new, functional, debug-draw-toggling code (this used to do terrain edge view)
            case SDL_SCANCODE_F1: publish("key_down_f1"); break;
            case SDL_SCANCODE_F2: publish("key_down_f2"); break;
            case SDL_SCANCODE_F3: publish("key_down_f3"); break;
            case SDL_SCANCODE_F4: publish("key_down_f4"); break;
            case SDL_SCANCODE_F5: publish("key_down_f5"); break;
            case SDL_SCANCODE_F6: publish("key_down_f6"); break;
            case SDL_SCANCODE_F7: publish("key_down_f7"); break;
            case SDL_SCANCODE_F8: publish("key_down_f8"); break;
            case SDL_SCANCODE_F9: publish("key_down_f9"); break;
            case SDL_SCANCODE_F10: publish("key_down_f10"); break;
            case SDL_SCANCODE_F11: publish("key_down_f11"); break;
            case SDL_SCANCODE_F12: publish("key_down_f12"); break;

            case SDL_SCANCODE_0: publish("key_down_0"); break;
            case SDL_SCANCODE_1: publish("key_down_1"); break;
            case SDL_SCANCODE_2: publish("key_down_2"); break;
            case SDL_SCANCODE_3: publish("key_down_3"); break;
            case SDL_SCANCODE_4: publish("key_down_4"); break;
            case SDL_SCANCODE_5: publish("key_down_5"); break;
            case SDL_SCANCODE_6: publish("key_down_6"); break;
            case SDL_SCANCODE_7: publish("key_down_7"); break;
            case SDL_SCANCODE_8: publish("key_down_8"); break;
            case SDL_SCANCODE_9: publish("key_down_9"); break;
            case SDL_SCANCODE_F: publish("key_down_f"); break;
            case SDL_SCANCODE_R: publish("key_down_r"); break;
            case SDL_SCANCODE_SPACE: publish("key_down_space"); break;
            default: break;
          } break;
        case SDL_MOUSEBUTTONDOWN:
          switch(event.button.button) {
            case SDL_BUTTON_LEFT: publish("mouse_down_left"); break;
            case SDL_BUTTON_RIGHT: publish("mouse_down_right"); break;
            case SDL_BUTTON_MIDDLE: publish("mouse_down_middle"); break;
            case SDL_BUTTON_X1: publish("mouse_down_x1"); break;
            case SDL_BUTTON_X2: publish("mouse_down_x2"); break;
            default: break;
          } break;
        case SDL_MOUSEMOTION:
          if (SDL_GetRelativeMouseMode()) {
            publish<SDL_Event>("mouse_moved", event);
          } break;
        case SDL_MOUSEWHEEL: {
          publish<int32_t>("mouse_wheel", event.wheel.y);
        } break;
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              settings::graphics::windowDimX = (uint32_t) event.window.data1;
              settings::graphics::windowDimY = (uint32_t) event.window.data2;
              // For some reason, this publication is not necessary on Windows, but is necessary on my Linux wm (bspwm).
              // Normally, Vulkan would catch the window resize by setting surface flags, but that is apparently not
              // as universal as it should be. In any case, this redundant call to window resize routines is fine.
              rtu::topics::publish("window_resized");
            } break;
#           if AT3_DEBUG_WINDOW_EVENTS
#             define REPORT_WINDOW_EVENT(e) printf("Window Event: %s\n", #e)
#           else
#             define REPORT_WINDOW_EVENT(e)
#           endif
            case SDL_WINDOWEVENT_MOVED: {
              settings::graphics::windowPosX = event.window.data1;
              settings::graphics::windowPosY = event.window.data2;
              REPORT_WINDOW_EVENT(MOVED);
            } break;
            case SDL_WINDOWEVENT_MAXIMIZED: {
              settings::graphics::fullscreen = settings::graphics::MAXIMIZED;
              REPORT_WINDOW_EVENT(MAXIMIZED);
            } break;
            case SDL_WINDOWEVENT_RESTORED: {
              settings::graphics::fullscreen = settings::graphics::WINDOWED;
              REPORT_WINDOW_EVENT(RESTORED);
            } break;
#           if AT3_DEBUG_WINDOW_EVENTS
#             define CASE_REPORT_WINDOW_EVENT(e) case SDL_WINDOWEVENT_ ##e: REPORT_WINDOW_EVENT(e); break
                CASE_REPORT_WINDOW_EVENT(SHOWN);
                CASE_REPORT_WINDOW_EVENT(HIDDEN);
                CASE_REPORT_WINDOW_EVENT(EXPOSED);
                CASE_REPORT_WINDOW_EVENT(RESIZED);
                CASE_REPORT_WINDOW_EVENT(MINIMIZED);
                CASE_REPORT_WINDOW_EVENT(ENTER);
                CASE_REPORT_WINDOW_EVENT(LEAVE);
                CASE_REPORT_WINDOW_EVENT(FOCUS_GAINED);
                CASE_REPORT_WINDOW_EVENT(FOCUS_LOST);
                CASE_REPORT_WINDOW_EVENT(CLOSE);
                CASE_REPORT_WINDOW_EVENT(TAKE_FOCUS);
                CASE_REPORT_WINDOW_EVENT(HIT_TEST);
#             undef CASE_REPORT_WINDOW_EVENT
#           endif
#           undef REPORT_WINDOW_EVENT
            default: break;
          } break;
        default: break;
      }
    }
  }

  void SdlContext::publishStates() {
    // let all know that a new state poll has begun, in case that information is useful
    publish("new_input_state_poll");

    // publish current keyboard state
    const Uint8 *keyStates = SDL_GetKeyboardState(NULL);
    RTU_DO_ON_KEYS(publish("key_held_w"), keyStates, SDL_SCANCODE_W)
    RTU_DO_ON_KEYS(publish("key_held_s"), keyStates, SDL_SCANCODE_S)
    RTU_DO_ON_KEYS(publish("key_held_d"), keyStates, SDL_SCANCODE_D)
    RTU_DO_ON_KEYS(publish("key_held_a"), keyStates, SDL_SCANCODE_A)
    RTU_DO_ON_KEYS(publish("key_held_e"), keyStates, SDL_SCANCODE_E)
    RTU_DO_ON_KEYS(publish("key_held_q"), keyStates, SDL_SCANCODE_Q)
    RTU_DO_ON_KEYS(publish("key_held_f"), keyStates, SDL_SCANCODE_F)
    RTU_DO_ON_KEYS(publish("key_held_c"), keyStates, SDL_SCANCODE_C)
    RTU_DO_ON_KEYS(publish("key_held_space"), keyStates, SDL_SCANCODE_SPACE)
    RTU_DO_ON_KEYS(publish("key_held_lshift"), keyStates, SDL_SCANCODE_LSHIFT)
    RTU_DO_ON_KEYS(publish("key_held_lalt"), keyStates, SDL_SCANCODE_LALT)
    RTU_DO_ON_KEYS(publish("key_held_lctrl"), keyStates, SDL_SCANCODE_LCTRL)
    RTU_DO_ON_KEYS(publish("key_held_f1"), keyStates, SDL_SCANCODE_F1)
    RTU_DO_ON_KEYS(publish("key_held_f2"), keyStates, SDL_SCANCODE_F2)
    RTU_DO_ON_KEYS(publish("key_held_f3"), keyStates, SDL_SCANCODE_F3)
    RTU_DO_ON_KEYS(publish("key_held_f4"), keyStates, SDL_SCANCODE_F4)
    RTU_DO_ON_KEYS(publish("key_held_f5"), keyStates, SDL_SCANCODE_F5)
    RTU_DO_ON_KEYS(publish("key_held_f6"), keyStates, SDL_SCANCODE_F6)
    RTU_DO_ON_KEYS(publish("key_held_f7"), keyStates, SDL_SCANCODE_F7)
    RTU_DO_ON_KEYS(publish("key_held_f8"), keyStates, SDL_SCANCODE_F8)
    RTU_DO_ON_KEYS(publish("key_held_f9"), keyStates, SDL_SCANCODE_F9)
    RTU_DO_ON_KEYS(publish("key_held_f10"), keyStates, SDL_SCANCODE_F10)
    RTU_DO_ON_KEYS(publish("key_held_f11"), keyStates, SDL_SCANCODE_F11)
    RTU_DO_ON_KEYS(publish("key_held_f12"), keyStates, SDL_SCANCODE_F12)

    // publish current mouse state
    const uint32_t mouseState = SDL_GetMouseState(nullptr, nullptr);
    if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) publish("mouse_held_left");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) publish("mouse_held_right");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) publish("mouse_held_middle");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_X1)) publish("mouse_held_x1");
    if (mouseState & SDL_BUTTON(SDL_BUTTON_X2)) publish("mouse_held_x2");
  }

  float SdlContext::getDeltaTime() {
    uint64_t currentTime = SDL_GetPerformanceCounter();
    uint64_t dt = currentTime - lastTime;
    lastTime = currentTime;

    double dtFloatSeconds = (double)dt * toSeconds;

    frameTime += dtFloatSeconds;
    ++frameCounter;
    if (frameTime > 10.f) {
      float frameTimeAvg = frameTime / (float)frameCounter;
      printf("AVG FPS: %.0f\n", 1.f / frameTimeAvg);
      frameTime = 0.f;
      frameCounter = 0;
    }

    return (float)dtFloatSeconds; // TODO: should keep double precision?
  }

}
