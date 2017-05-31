
#include "settings.h"

namespace at3 {
  namespace settings {

    // These are default settings. They can be overwritten in the settings ini file.
    namespace graphics {
      uint32_t api = OPENGL;
      uint32_t windowWidth = 800;
      uint32_t windowHeight = 600;
      float fovy = 1.1f;
      float near = 0.1f;
      float far = 10000.f;
    }

    // This is the mapping from the names that appear in the ini to the variables here.
    void setupRegistries() {
      registry.insert(std::make_pair("u_graphicsApi", &graphics::api));
      registry.insert(std::make_pair("u_windowWidth", &graphics::windowWidth));
      registry.insert(std::make_pair("u_windowHeight", &graphics::windowHeight));
      registry.insert(std::make_pair("f_fieldOfViewY", &graphics::fovy));
      registry.insert(std::make_pair("f_nearPlane", &graphics::near));
      registry.insert(std::make_pair("f_farPlane", &graphics::far));
      for (auto pair : registry) {
        reverseRegistry.insert(std::make_pair(pair.second, pair.first));
      }
    }

    bool loadFromIni(const char *filename) {
      setupRegistries();
      std::ifstream ini(filename);
      if (!ini) {
        fprintf(stderr, "SETTINGS: Could not open file: %s\n", filename);
        return false;
      }
      std::string settingName, garbage;
      bool workedFlawlessly = true;
      while (ini >> settingName) {
        if (registry.count(settingName)) {
          char type = settingName[0];
          switch (type) {
            case 'u': {
              ini >> (*((uint32_t *) registry.at(settingName)));
            } break;
            case 'f': {
              ini >> (*((float *) registry.at(settingName)));
            } break;
            default: {
              fprintf(stderr, "SETTINGS: Attempted to set with invalid type prefix: %s\n", settingName.c_str());
              workedFlawlessly = false;
            }
          }
          fprintf(stdout, "Loading setting: %s\n", settingName.c_str());
        } else {
          fprintf(stderr, "SETTINGS: Unrecognized input: %s\n", settingName.c_str());
          workedFlawlessly = false;
        }
      }
      if (!workedFlawlessly) {
        fprintf(stderr, "SETTINGS: Some settings failed to load!\n");
      }
      return workedFlawlessly;
    }

    void saveToIni(const char *filename) {
      // TODO
    }

    std::unordered_map<std::string, void*> registry;
    std::unordered_map<void*, std::string> reverseRegistry;
    bool loaded = loadFromIni("settings.ini");
  }
}
