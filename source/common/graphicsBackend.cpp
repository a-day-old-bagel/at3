
#include <iostream>
#include "graphicsBackend.h"

namespace at3 {
  namespace graphicsBackend {

    bool init() {
	    currentFovY = settings::graphics::fovy;
      bool success = true;
      success &= sdl::init();
      switch (settings::graphics::api) {
        case settings::graphics::OPENGL: success &= opengl::init(); break;
        case settings::graphics::VULKAN: success &= vulkan::init(); break;
        default: {
          std::cerr << "Invalid graphics API selection: " << settings::graphics::api << std::endl;
          success = false;
        }
      }
      if(success) {
        Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);
        setFullscreenMode(settings::graphics::fullscreen);
      }
      return success;
    }
    void swap() {
      switch (settings::graphics::api) {
        case settings::graphics::OPENGL: opengl::swap(); break;
        case settings::graphics::VULKAN: vulkan::swap(); break;
        default: assert(false);
      }
    }
    void clear() {
      switch (settings::graphics::api) {
        case settings::graphics::OPENGL: opengl::clear(); break;
        case settings::graphics::VULKAN: vulkan::clear(); break;
        default: assert(false);
      }
    }
    bool handleEvent(const SDL_Event &event) {
      switch (event.type) {
        case SDL_WINDOWEVENT:
          switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
              settings::graphics::windowDimX = (uint32_t) event.window.data1;
              settings::graphics::windowDimY = (uint32_t) event.window.data2;
              glViewport(0, 0, settings::graphics::windowDimX, settings::graphics::windowDimY);
              Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);
              std::cout << "Window size changed to: " << settings::graphics::windowDimX << "x"
                        << settings::graphics::windowDimY << std::endl;
            } break;
            case SDL_WINDOWEVENT_MOVED: {
              settings::graphics::windowPosX = event.window.data1;
              settings::graphics::windowPosY = event.window.data2;
            } break;
            case SDL_WINDOWEVENT_MAXIMIZED: {
              settings::graphics::fullscreen = settings::graphics::MAXIMIZED;
            } break;
            case SDL_WINDOWEVENT_RESTORED: {
              settings::graphics::fullscreen = settings::graphics::WINDOWED;
            } break;
            default: break;
          } break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_F: {
              toggleFullscreen();
            } break;
            case SDL_SCANCODE_V: {
              Shaders::toggleEdgeView();
            } break;
            default: break;
          } return false;       // did not handle it here
        default: return false;  // did not handle it here
      }
      return true;              // handled it here
    }
    bool setFullscreenMode(uint32_t mode) {
      switch (mode) {
        case settings::graphics::FULLSCREEN: {
          SDL_SetWindowFullscreen(sdl::window, SDL_WINDOW_FULLSCREEN);
        } break;
        case settings::graphics::FAKED_FULLSCREEN: {
          SDL_SetWindowFullscreen(sdl::window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } break;
        case settings::graphics::MAXIMIZED: SDL_MaximizeWindow(sdl::window);
        case settings::graphics::WINDOWED:
        default: {
          SDL_SetWindowFullscreen(sdl::window, 0);
        }
      }
      bool currentlyFull = (bool) (SDL_GetWindowFlags(sdl::window) & SDL_WINDOW_FULLSCREEN);
      bool currentlyFake = (bool) (SDL_GetWindowFlags(sdl::window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
      if (currentlyFull) {
        settings::graphics::fullscreen = settings::graphics::FULLSCREEN;
      } else if (currentlyFake) {
        settings::graphics::fullscreen = settings::graphics::FAKED_FULLSCREEN;
      } else {
        settings::graphics::fullscreen = settings::graphics::WINDOWED;
      }
      return (settings::graphics::fullscreen == mode);
    }
    void toggleFullscreen() {
      bool currentlyFull = (bool) (SDL_GetWindowFlags(sdl::window) & SDL_WINDOW_FULLSCREEN);
      bool currentlyFake = (bool) (SDL_GetWindowFlags(sdl::window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
      if (currentlyFull || currentlyFake) {
        if ( ! setFullscreenMode(settings::graphics::WINDOWED)) {
          std::cerr << "Failed to change to windowed mode!" << std::endl;
        }
      } else {
        if ( ! setFullscreenMode(settings::graphics::FULLSCREEN)) {
          std::cerr << "Failed to change to fullscreen mode!" << std::endl;
        }
      }
    }
    float getAspect() {
      return (float)settings::graphics::windowDimX / (float)settings::graphics::windowDimY;
    }
    void setFovy(float fovy) {
      graphicsBackend::currentFovY = fovy;
      Shaders::updateViewInfos(fovy, settings::graphics::windowDimX, settings::graphics::windowDimY);
    }

    float currentFovY;

    namespace sdl {
      bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
          fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
          return false;
        }
        switch (settings::graphics::fullscreen) {
          case settings::graphics::FAKED_FULLSCREEN: {
            windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
          } break;
          case settings::graphics::FULLSCREEN: {
            windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
          } break;
          case settings::graphics::WINDOWED:
          default: break;
        }
        return true;
      }
      SDL_Window *window = NULL;
      const char *windowTitle = NULL;
      uint32_t windowFlags = SDL_WINDOW_RESIZABLE;
    }

    namespace opengl {
      bool init() {
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        sdl::window = SDL_CreateWindow(
            sdl::windowTitle,                      // title
            settings::graphics::windowPosX,        // x
            settings::graphics::windowPosY,        // y
            settings::graphics::windowDimX,        // width
            settings::graphics::windowDimY,        // height
            sdl::windowFlags | SDL_WINDOW_OPENGL   // flags
        );
        if (sdl::window == nullptr) {
          fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
          return false;
        }
        glContext = SDL_GL_CreateContext(sdl::window);
        if (glContext == nullptr) {
          fprintf(stderr, "Failed to initialize OpenGL context: %s\n", SDL_GetError());
          return false;
        }
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepth(1.0);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glViewport(0, 0, settings::graphics::windowDimX, settings::graphics::windowDimY);
        ASSERT_GL_ERROR();
        return true;
      }
      void swap() {
        SDL_GL_SwapWindow(sdl::window);
      }
      void clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }
      SDL_GLContext glContext;
    }

    namespace vulkan {
      bool init() {
        fprintf(stderr, "Vulkan API not yet supported!\n");
        return false;
      }
      void swap() {

      }
      void clear() {

      }
    }
  }
}
