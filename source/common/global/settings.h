//
// Created by volundr on 5/30/17.
//

#pragma once

#include <fstream>
#include <string>
#include <map>

namespace at3 {
  namespace settings {

    namespace graphics {
      enum Api {
        INVALID_API = 0,
        SDL2 = 1, GLFW = 2, OPENGL_CL = 3, VULKAN = 4
      };
      enum ScreenMode {
        INVALID_MODE = 0,
        WINDOWED = 1, MAXIMIZED = 2, FAKED_FULLSCREEN = 3, FULLSCREEN = 4
      };
      extern uint32_t gpuApi, windowApi;
      extern uint32_t fullscreen;
      extern uint32_t windowDimX, windowDimY;
      extern int32_t windowPosX, windowPosY;
      extern float fovy, near, far;
    }
    void setupRegistries();
    bool loadFromIni(const char *filename);
    bool saveToIni(const char *filename);
    extern std::map<std::string, void *> registry;
    extern bool loaded;
  }
}
