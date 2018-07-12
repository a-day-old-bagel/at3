
#pragma once

#include <SDL.h>

namespace at3 {
  class SdlContext {
    public:
      explicit SdlContext(const char *applicationName);
      SDL_Window * getWindow();
      bool setFullscreenMode(uint32_t mode);
      void toggleFullscreen(void *nothing);
    private:
      SDL_Window *window = nullptr;
      uint32_t windowFlags = 0;
  };
}
