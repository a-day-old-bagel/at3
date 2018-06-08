
#include <iostream>
#include "graphicsBackend.hpp"

namespace at3 {
  namespace graphicsBackend {
    namespace opengl {
      bool init() {
        glContext = SDL_GL_CreateContext(window);
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
        SDL_GL_SwapWindow(window);
      }
      void clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      }
      SDL_GLContext glContext;
    }
  }
}
