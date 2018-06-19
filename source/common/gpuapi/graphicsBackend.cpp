
#include <iostream>
#include "graphicsBackend.hpp"

using namespace rtu::topics;

namespace at3 {
  namespace graphicsBackend {

    bool init(SDL_Window *window) {
      graphicsBackend::window = window;
	    currentFovY = settings::graphics::fovy;
      bool success = opengl::init();
      if(success) {
        Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);
        setFullscreenMode(settings::graphics::fullscreen);
      }
      return success;
    }
    void swap() {
      opengl::swap();
    }
    void clear() {
      opengl::clear();
    }
    void deInit() {
      // TODO: clean up SDL, GLFW, OPENGL, VULKAN, ETC!!! (in their own places)
      std::cout << "GPU/Window resource clean-up needs implementation." << std::endl;
    }

    bool setFullscreenMode(uint32_t mode) {
      switch (mode) {
        case settings::graphics::FULLSCREEN: {
          SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        } break;
        case settings::graphics::FAKED_FULLSCREEN: {
          SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } break;
        case settings::graphics::MAXIMIZED: SDL_MaximizeWindow(window);
        case settings::graphics::WINDOWED:
        default: {
          SDL_SetWindowFullscreen(window, 0);
        }
      }
      auto isCurrentlyFull = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
      auto isCurrentlyFake = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
      if (isCurrentlyFull) {
        settings::graphics::fullscreen = settings::graphics::FULLSCREEN;
      } else if (isCurrentlyFake) {
        settings::graphics::fullscreen = settings::graphics::FAKED_FULLSCREEN;
      } else {
        settings::graphics::fullscreen = settings::graphics::WINDOWED;
      }
      return (settings::graphics::fullscreen == mode);
    }

    void toggleFullscreen(void *nothing) {
      auto isCurrentlyFull = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN);
      auto isCurrentlyFake = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP);
      if (isCurrentlyFull || isCurrentlyFake) {
        if ( ! setFullscreenMode(settings::graphics::WINDOWED)) {
          std::cerr << "Failed to change to windowed mode!" << std::endl;
        }
      } else {
        if ( ! setFullscreenMode(settings::graphics::FAKED_FULLSCREEN)) {
          std::cerr << "Failed to change to windowed fullscreen mode!" << std::endl;
        }
      }
    }

    void handleWindowEvent(void *windowEvent) {
      auto *event = (SDL_Event*)windowEvent;
      switch (event->window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
          settings::graphics::windowDimX = (uint32_t) event->window.data1;
          settings::graphics::windowDimY = (uint32_t) event->window.data2;
          glViewport(0, 0, settings::graphics::windowDimX, settings::graphics::windowDimY);
          Shaders::updateViewInfos(currentFovY, settings::graphics::windowDimX, settings::graphics::windowDimY);
          std::cout << "Window size changed to: " << settings::graphics::windowDimX << "x"
                    << settings::graphics::windowDimY << std::endl;
        } break;
        case SDL_WINDOWEVENT_MOVED: {
          settings::graphics::windowPosX = event->window.data1;
          settings::graphics::windowPosY = event->window.data2;
        } break;
        case SDL_WINDOWEVENT_MAXIMIZED: {
          settings::graphics::fullscreen = settings::graphics::MAXIMIZED;
        } break;
        case SDL_WINDOWEVENT_RESTORED: {
          settings::graphics::fullscreen = settings::graphics::WINDOWED;
        } break;
        default: break;
      }
    }

    float getAspect() {
      return (float)settings::graphics::windowDimX / (float)settings::graphics::windowDimY;
    }
    void setFovy(float fovy) {
      graphicsBackend::currentFovY = fovy;
      Shaders::updateViewInfos(fovy, settings::graphics::windowDimX, settings::graphics::windowDimY);
    }

    SDL_Window *window = nullptr;
    float currentFovY;
  }
}
