
#pragma once

#include "definitions.hpp"

#include <fstream>
#include <string>
#include <map>
#include <vector>

namespace at3 {
  namespace settings {

    namespace graphics {
      enum ScreenMode {
        INVALID_MODE = 0,
        WINDOWED = 1, MAXIMIZED = 2, FAKED_FULLSCREEN = 3, FULLSCREEN = 4
      };
      extern uint32_t fullscreen;
      extern uint32_t windowDimX, windowDimY;
      extern int32_t windowPosX, windowPosY;
      extern float fovy;

      namespace vulkan {
        extern bool forceFifo;
      }
    }

    namespace controls {
      extern float mouseSpeed;
      extern bool mouseInvertX;
      extern bool mouseInvertY;
    }

    namespace network {
      enum Role {
          NONE,
          SERVER,
          CLIENT
      };
      extern uint32_t role;
      extern std::string serverAddress;
      extern uint32_t serverPort;
      extern uint32_t maxServerConns;
    }

    bool addCustom(const char *name, void *data);
    bool loadFromIni(const char *filename);
    bool saveToIni(const char *filename);
  }
}
