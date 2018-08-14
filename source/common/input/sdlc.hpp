
#pragma once

#include <SDL.h>
#include "math.hpp"

#if !SDL_VERSION_ATLEAST(2, 0, 8)
#error "SDL2 version 2.0.8 or newer required."
#endif

namespace at3 {
  class SdlContext {
    public:
      explicit SdlContext(const char *applicationName);
      SDL_Window * getWindow();
      bool setFullscreenMode(uint32_t mode);
      void toggleFullscreen(void *nothing);
      void publishEvents();
      float getDeltaTime();
    private:
      SDL_Window *window = nullptr;
      uint32_t windowFlags = 0;
      float lastTime = (float)SDL_GetTicks() * msToS;
  };
}
