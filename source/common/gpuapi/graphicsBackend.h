
#pragma once

#include <epoxy/gl.h>
#include <SDL_vulkan.h>
#include <SDL.h>

#include "settings.h"
#include "loadedTexture.h"
#include "material.h"
#include "openglValidation.h"
#include "shaders.h"
#include "shaderProgram.h"

namespace at3 {
  namespace graphicsBackend {
    bool init();
    void swap();
    void clear();
    void deInit();
    bool handleEvent(const SDL_Event &event);
    bool setFullscreenMode(uint32_t mode);
    void toggleFullscreen();
    float getAspect();
    void setFovy(float fovy);

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

      extern PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
      extern PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
      extern VkDebugReportCallbackEXT msg_callback;
      extern PFN_vkDebugReportMessageEXT DebugReportMessage;
    }
  }
}
