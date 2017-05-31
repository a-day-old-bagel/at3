
#include <iostream>
#include "graphicsBackend.h"

namespace at3 {
  namespace graphicsBackend {
    bool init() {
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
              currentWindowWidth = event.window.data1;
              currentWindowHeight = event.window.data2;
              glViewport(0, 0, currentWindowWidth, currentWindowHeight);
              Shaders::updateViewInfos(currentFovY, currentWindowWidth, currentWindowHeight);
              std::cout << "NEW SCREEN SIZE: " << currentWindowWidth << ", " << currentWindowHeight << std::endl;
              break;
            }
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
          } return false; // could not handle it here
        default: return false;  // could not handle it here
      }
      return true;  // handled it here
    }
    bool toggleFullscreen() {
      bool isFullScreen = (bool)(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
      SDL_SetWindowFullscreen(window, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN);
      return !isFullScreen;
    }
    float getAspect() {
      return (float)currentWindowWidth / (float)currentWindowHeight;
    }
    void setFovy(float fovy) {
      graphicsBackend::currentFovY = fovy;
      Shaders::updateViewInfos(fovy, currentWindowWidth, currentWindowHeight);
    }

    const char *windowTitle;
    SDL_Window *window;
    int currentWindowWidth;
    int currentWindowHeight;
    float currentFovY;

    namespace opengl {
      bool init() {

        // SDL2 stuff
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
          fprintf(stderr, "Failed to initialize SDL: %s\n",
                  SDL_GetError());
          return false;
        }
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        graphicsBackend::window = SDL_CreateWindow(
            graphicsBackend::windowTitle,            // title
            SDL_WINDOWPOS_UNDEFINED,  // x
            SDL_WINDOWPOS_UNDEFINED,  // y
            settings::graphics::windowWidth, settings::graphics::windowHeight,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE  // flags
        );
        if (graphicsBackend::window == nullptr) {
          fprintf(stderr, "Failed to create SDL window: %s\n",
                  SDL_GetError());
          return false;
        }

        // OpenGL stuff
        glContext = SDL_GL_CreateContext(graphicsBackend::window);
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
        glViewport(0, 0, settings::graphics::windowWidth, settings::graphics::windowHeight);
        ASSERT_GL_ERROR();

        Shaders::updateViewInfos(currentFovY, currentWindowWidth, currentWindowHeight);

        return true;
      }
      void swap() {
        SDL_GL_SwapWindow(graphicsBackend::window);
      }
      void clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }
      const char *windowTitle = NULL;
      SDL_Window *window = NULL;
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
