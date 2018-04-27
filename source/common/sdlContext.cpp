
#include "sdlContext.h"
#include "settings.h"

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
    switch (settings::graphics::gpuApi) {
      case settings::graphics::VULKAN: {

      } break;
      case settings::graphics::OPENGL_OPENCL: {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        windowFlags |= SDL_WINDOW_OPENGL;
      } break;
      default: {
        fprintf(stderr, "Invalid graphics API selection: %u\n", settings::graphics::gpuApi);
      }
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
}
