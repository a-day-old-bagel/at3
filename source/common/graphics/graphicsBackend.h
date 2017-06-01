
#pragma once

#include <epoxy/gl.h>
#include <SDL.h>
#include "GLFW/glfw3.h"

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
    bool setFullscreenMode(uint32_t mode);
    void toggleFullscreen();
    float getAspect();
    void setFovy(float fovy);

    extern float currentFovY;

    namespace glfw {
      bool init();
      extern GLFWwindow* window;
    }
    namespace sdl2 {
      bool init();
      extern SDL_Window *window;
      extern const char *windowTitle;
      extern uint32_t windowFlags;
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
