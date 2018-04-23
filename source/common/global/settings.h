
#pragma once

#include <fstream>
#include <string>
#include <map>
#include <vector>

#include "configuration.h"

namespace at3 {
  namespace settings {

    namespace graphics {
      enum Api {
        INVALID_API = 0,
        OPENGL_OPENCL = 5, VULKAN = 6
      };
      enum ScreenMode {
        INVALID_MODE = 0,
        WINDOWED = 1, MAXIMIZED = 2, FAKED_FULLSCREEN = 3, FULLSCREEN = 4
      };
      extern uint32_t gpuApi;
      extern uint32_t fullscreen;
      extern uint32_t windowDimX, windowDimY;
      extern int32_t windowPosX, windowPosY;
      extern float fovy, near, far;

      namespace vulkan {
        extern bool forceFifo;
      }
    }

    namespace controls {
      extern float mouseSpeed;
    }

    bool addCustom(const char *name, void *data);
    bool loadFromIni(const char *filename);
    bool saveToIni(const char *filename);
  }
}
