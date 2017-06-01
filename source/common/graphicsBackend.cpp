
#include <iostream>
#include "graphicsBackend.h"

namespace at3 {
  namespace graphicsBackend {
    bool init() {
	  currentFovY = settings::graphics::fovy;
      switch (settings::graphics::api) {
        case settings::graphics::OPENGL: return opengl::init();
        case settings::graphics::VULKAN: return vulkan::init();
        default: return false;
      }
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
              // TODO: SDL_SetWindowDisplayMode
              settings::graphics::windowDimX = (uint32_t)event.window.data1;
              settings::graphics::windowDimY = (uint32_t)event.window.data2;
              glViewport(0, 0, settings::graphics::windowDimX, settings::graphics::windowDimY);
              Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);
              std::cout << "NEW SCREEN SIZE: " << settings::graphics::windowDimX
                        << ", " << settings::graphics::windowDimY << std::endl;
            } break;
            case SDL_WINDOWEVENT_MOVED: {
              // TODO: SDL_SetWindowDisplayMode
            } break;
            default: break;
          } break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_F: {
              if (toggleFullscreen()) {
                std::cout << "Fullscreen ON." << std::endl;
              } else {
                std::cout << "Fullscreen OFF." << std::endl;
              }
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
    bool toggleFullscreen() {
      bool isFullScreen = (bool)(SDL_GetWindowFlags(sdl::window) & SDL_WINDOW_FULLSCREEN);
      SDL_SetWindowFullscreen(sdl::window, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN);
      return !isFullScreen;
    }
    float getAspect() {
      return (float)settings::graphics::windowDimX / (float)settings::graphics::windowDimY;
    }
    void setFovy(float fovy) {
      graphicsBackend::currentFovY = fovy;
      Shaders::updateViewInfos(fovy, settings::graphics::windowDimX, settings::graphics::windowDimY);
    }

    const char *windowTitle = NULL;
    float currentFovY;

    namespace sdl {
      SDL_Window *window = NULL;
    }

    namespace opengl {
      bool init() {

        // SDL2 stuff
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
          fprintf(stderr, "Failed to initialize SDL: %s\n",
                  SDL_GetError());
          return false;
        }
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        sdl::window = SDL_CreateWindow(
            graphicsBackend::windowTitle,            // title
            SDL_WINDOWPOS_UNDEFINED,  // x
            SDL_WINDOWPOS_UNDEFINED,  // y
            settings::graphics::windowDimX, settings::graphics::windowDimY,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE  // flags
        );
        if (sdl::window == nullptr) {
          fprintf(stderr, "Failed to create SDL window: %s\n",
                  SDL_GetError());
          return false;
        }

        // OpenGL stuff
        glContext = SDL_GL_CreateContext(sdl::window);
        if (glContext == nullptr) {
          fprintf(stderr, "Failed to initialize OpenGL context: %s\n",
                  SDL_GetError());
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

        Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);

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
