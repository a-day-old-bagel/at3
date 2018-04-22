
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
#include "vulkanBackend.h"
#include "vulkanContext.h"

namespace at3 {

  struct FakeScene {

  };

  namespace graphicsBackend {
    bool init();
    void swap();
    void clear();
    void deInit();
    bool setFullscreenMode(uint32_t mode);
    float getAspect();
    void setFovy(float fovy);

    void toggleFullscreen(void *nothing);
    void handleWindowEvent(void *windowEvent);

    extern const char *applicationName;
    extern float currentFovY;

    namespace sdl2 {
      bool init();
      extern SDL_Window *window;
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
      void handleWindowSizeChange();
      extern std::unique_ptr<VulkanBackend> vkbe;
//      extern std::unique_ptr<VulkanContext<FakeScene>> vkc;
    }
  }
}
