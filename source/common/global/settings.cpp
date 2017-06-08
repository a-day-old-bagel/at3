
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

    // The settings registry contains mappings from setting name to memory location.
    std::map<std::string, void*> registry, customRegistry;
    // This indicates whether an attempt to load settings has yet been made.
    bool loaded = false;

    /**
     * addCustomSetting
     * defines a desired mapping from a name that might appear in the ini to a location
     * in memory. It should go without saying that these memory addresses must remain
     * valid throughout the duration of time between the loadFromIni call to the
     * final saveToIni call, presumably about the entire lifetime of the program.
     *
     * When an ini is saved, the entries will be arranged alphabetically.
     * A variable name in the ini must be formatted such that the LAST LETTER of the
     * name is one of the following (to add to this list, make an entry in the switch
     * statements that appear in the functions 'loadFromIni' and 'saveToIni' in this file):
     *      'u' : indicates that the variable will be a 'uint32_t' in memory
     *      'i' : indicates that the variable will be a 'int32_t' in memory
     *      'f' : indicates that the variable will be a 'float' in memory
     *
     * This operation will fail if the desired setting name is already taken or if
     * the settings file has already been loaded.
     *
     * @param name will appear as a term in the settings ini file
     * @param data must point to a valid location in memory of matching type
     * @return true if successful
     */
    bool addCustom(const char *name, void *data) {
      if (loaded) {
        return false;
      } else {
        return customRegistry.insert(std::make_pair( name, data )).second;
      }
    }

    /**
     * Defines the mapping from names that appear in the ini to the standard
     * settings variables declared in settings.h. These settings will be registered
     * before any custom settings.
     */
    void setupRegistry() {
      registry.insert(std::make_pair( "graphics_api_gpu_u", &graphics::gpuApi));
      registry.insert(std::make_pair( "graphics_fullscr_u", &graphics::fullscreen));
      registry.insert(std::make_pair( "graphics_win_dimx_u", &graphics::windowDimX));
      registry.insert(std::make_pair( "graphics_win_dimy_u", &graphics::windowDimY));
      registry.insert(std::make_pair( "graphics_win_posx_i", &graphics::windowPosX));
      registry.insert(std::make_pair( "graphics_win_posy_i", &graphics::windowPosY));

      // Now add the custom entries after - they will never overwrite the standard ones.
      registry.insert(customRegistry.begin(), customRegistry.end());
      customRegistry.clear();
    }

    // This is used in the functions below.
#   define CASE_CHAR_TYPE(c, t) case c: ini >> (*(( t *) registry.at(settingName))); break;

    /**
     * Attempts to read a user settings ini file. This call is designed to only be made once
     * per run of the application. Any further attempts after the first will fail.
     * @param filename of the ini file
     * @return true if worked, false if any problems (fatal or otherwise) were encountered
     */
    bool loadFromIni(const char *filename) {
      std::stringstream reportProblem, reportNormal;
      bool workedFlawlessly = true;
      if (loaded) {
        reportProblem << "An attempt to load settings has already been made" << std::endl;
        workedFlawlessly = false;
      } else {
        loaded = true;
        setupRegistry();
        std::ifstream ini(filename);
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
                }
                  continue;
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

  }
}
