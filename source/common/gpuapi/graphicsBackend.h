
#pragma once

#include <epoxy/gl.h>
#include <SDL.h>

#include "settings.h"
#include "loadedTexture.h"
#include "material.h"
#include "openglValidation.h"
#include "shaders.h"
#include "shaderProgram.h"
#include "topics.hpp"

namespace at3 {
  namespace graphicsBackend {
    bool init(SDL_Window *window);
    void swap();
    void clear();
    void deInit();
    bool setFullscreenMode(uint32_t mode);
    float getAspect();
    void setFovy(float fovy);

    void toggleFullscreen(void *nothing);
    void handleWindowEvent(void *windowEvent);

    extern float currentFovY;
    extern SDL_Window *window;

    namespace opengl {
      bool init();
      void swap();
      void clear();
      extern SDL_GLContext glContext;
    }
  }
}
