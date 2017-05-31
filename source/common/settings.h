//
// Created by volundr on 5/30/17.
//

#pragma once

#include <fstream>
#include <string>
#include <unordered_map>

namespace at3 {
  namespace settings {

    namespace graphics {
      enum Api {
        OPENGL = 0, VULKAN = 1
      };
      extern uint32_t api;
      extern uint32_t windowWidth, windowHeight;
      extern float fovy, near, far;
    }

    void setupRegistries();
    bool loadFromIni(const char *filename);
    void saveToIni(const char *filename);
    extern std::unordered_map<std::string, void*> registry;
    extern std::unordered_map<void*, std::string> reverseRegistry;
    extern bool loaded;
  }
}
