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
        OPENGL = 0, VULKAN = 1
      };
      extern uint32_t api;
      extern uint32_t windowDimX, windowDimY;
      extern uint32_t windowPosX, windowPosY;
      extern uint32_t windowResX, windowResY;
      extern float fovy, near, far;
    }

    void setupRegistries();
    bool loadFromIni(const char *filename);
    bool saveToIni(const char *filename);
    extern std::map<std::string, void*> registry;
    extern bool loaded;
  }
}
