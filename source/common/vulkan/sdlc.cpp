
#include "sdlc.hpp"
#include "settings.hpp"

namespace at3 {
  SdlContext::SdlContext(const char *applicationName) {
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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
  void SdlContext::toggleFullscreen(void *nothing) {
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
}
