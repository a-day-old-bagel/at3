
#include <sstream>
#include <SDL_video.h>
#include "settings.h"

namespace at3 {
  namespace settings {

    // These are default settings. They can be overwritten at runtime with an ini file.
    namespace graphics {
      uint32_t gpuApi = OPENGL_OPENCL;
      uint32_t fullscreen = WINDOWED;
      uint32_t windowDimX = 1200;
      uint32_t windowDimY = 700;
      int32_t  windowPosX = SDL_WINDOWPOS_CENTERED;
      int32_t  windowPosY = SDL_WINDOWPOS_CENTERED;
      float fovy = 1.1f;
      float near = 0.1f;
      float far = 10000.f;
    }

    /**
     * Defines the mapping from the names that appear in the ini to the variables here.
     * When an ini is saved, the entries will be arranged alphabetically.
     * A variable name in the ini must be formatted such that the LAST LETTER of the
     * name is one of the following (to add to this list, make an entry in the switch
     * statement that appears in the functions 'loadFromIni' and 'saveToIni' below):
     *      'u' : indicates that the variable will be a 'uint32_t' in memory
     *      'i' : indicates that the variable will be a 'int32_t' in memory
     *      'f' : indicates that the variable will be a 'float' in memory
     */
    void setupRegistries() {
      registry.insert(std::make_pair( "graphics_api_gpu_u", &graphics::gpuApi));
      registry.insert(std::make_pair( "graphics_fullscr_u", &graphics::fullscreen));
      registry.insert(std::make_pair( "graphics_win_dimx_u", &graphics::windowDimX));
      registry.insert(std::make_pair( "graphics_win_dimy_u", &graphics::windowDimY));
      registry.insert(std::make_pair( "graphics_win_posx_i", &graphics::windowPosX));
      registry.insert(std::make_pair( "graphics_win_posy_i", &graphics::windowPosY));
    }

    // This is used in the functions below.
#   define CASE_CHAR_TYPE(c, t) case c: ini >> (*(( t *) registry.at(settingName))); break;

    /**
     * Attempts to read a user settings ini file.
     * @param filename of the ini file
     * @return true if worked, false if any problems (fatal or otherwise) were encountered
     */
    bool loadFromIni(const char *filename) {
      setupRegistries();
      std::ifstream ini(filename);
      std::stringstream reportProblem, reportNormal;
      bool workedFlawlessly = true;
      if (!ini) {
        reportProblem << "No user settings found (" << filename
               << "). Default settings will be used." << std::endl;
        workedFlawlessly = false;
      } else {
        reportNormal << "Found user settings file: " << filename << std::endl;
        std::string settingName, garbage;
        while (ini >> settingName) {
          if (registry.count(settingName)) {
            char lastLetter = (char) settingName[settingName.size() - 1];
            switch (lastLetter) {

              // Add an entry for any other types you want to support here and in 'saveToIni'.
              // make sure the type can be read from a stream with the >> operator correctly.
              CASE_CHAR_TYPE('u', uint32_t)
              CASE_CHAR_TYPE('i', int32_t)
              CASE_CHAR_TYPE('f', float)

              default: {
                reportProblem << "Encountered user setting with invalid type postfix: "
                              << settingName << std::endl;
                workedFlawlessly = false;
              } continue;
            }
            if (ini.eof()) {
              reportProblem << "Value missing for user setting: " << settingName << std::endl;
              continue;
            } else if (ini.fail()) {
              reportProblem << "Error reading user setting value: " << settingName << std::endl;
              ini.clear();
              continue;
            }
            reportNormal << "Loaded user setting: " << settingName << std::endl;
          } else {
            reportProblem << "Unrecognized user settings input: " << settingName << std::endl;
            workedFlawlessly = false;
          }
        }
      }
      if (!workedFlawlessly) {
        reportProblem << "Some user settings may have failed to load!" << std::endl;
      }
      fprintf(stdout, "\n%s%s\n", reportNormal.str().c_str(), reportProblem.str().c_str());
      return workedFlawlessly;
    }

    // This is being redefined for the function below.
#   undef CASE_CHAR_TYPE
#   define CASE_CHAR_TYPE(c, t) case c: ini << settingName << " " \
                                << (*(( t *) pair.second)) << std::endl; break;

    /**
     * Attempts to write a user settings ini file (will overwrite entire file).
     * @param filename of the ini file
     * @return true if worked, false if any problems (fatal or otherwise) were encountered
     */
    bool saveToIni(const char *filename) {
      std::ofstream ini(filename, std::ios_base::out);
      std::stringstream reportProblem, reportNormal;
      bool workedFlawlessly = true;
      for (auto pair : registry) {
        std::string settingName = pair.first;
        char lastLetter = (char) settingName[settingName.size() - 1];
        switch (lastLetter) {

          // Add an entry for any other types you want to support here and in 'loadFromIni'.
          // make sure the type can be written to a stream with the << operator correctly.
          CASE_CHAR_TYPE('u', uint32_t)
          CASE_CHAR_TYPE('i', int32_t)
          CASE_CHAR_TYPE('f', float)

          default: {
            reportProblem << "Did not save user setting (invalid type postfix): "
                          << settingName << " postfix was: " << lastLetter << std::endl;
            workedFlawlessly = false;
          } continue;
        }
        reportNormal << "Saved user setting: " << settingName << std::endl;
      }
      if (!workedFlawlessly) {
        reportProblem << "Some user settings may may not have been saved!" << std::endl;
      }
      fprintf(stdout, "\n%s%s", reportNormal.str().c_str(), reportProblem.str().c_str());
      return workedFlawlessly;
    }

    // This was used in the functions above.
#   undef CASE_CHAR_TYPE

    std::map<std::string, void*> registry;

    /**
     * 'settings::loaded' will be initialized to true if no problems were encountered while
     * loading settings from a user ini file
     */
    bool loaded = loadFromIni("settings.ini");
  }
}
