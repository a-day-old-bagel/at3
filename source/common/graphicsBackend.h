
#pragma once

#include <epoxy/gl.h>
#include <SDL.h>

#include "settings.h"
#include "loadedTexture.h"
#include "material.h"
#include "glUtil.h"
#include "shaders.h"
#include "shaderProgram.h"

namespace at3 {
  namespace graphicsBackend {
    bool init();
    void swap();
    void clear();
    bool handleEvent(const SDL_Event &event);
    bool toggleFullscreen();  // returns true if fullscreen is on after the call.
    float getAspect();
    void setFovy(float fovy);

    extern const char *windowTitle;
    extern float currentFovY;

    namespace sdl {
      extern SDL_Window *window;
    }
    namespace opengl {
      bool init();
      void swap();
      void clear();
      extern SDL_GLContext glContext;
    }
    namespace vulkan {
      bool init();
      void swap();
      void clear();
    }
  }
}
