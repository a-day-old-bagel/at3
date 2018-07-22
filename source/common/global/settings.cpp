
#include <sstream>
#include <SDL_video.h>
#include "settings.hpp"

#define AT3_VERBOSE_SETTINGS 0

namespace at3 {
  namespace settings {

    // Hard values given here are just default values. They are be overwritten at runtime with the ini file.
    namespace graphics {
      uint32_t fullscreen = WINDOWED;
      uint32_t windowDimX = 1200;
      uint32_t windowDimY = 700;
      int32_t  windowPosX = SDL_WINDOWPOS_CENTERED;
      int32_t  windowPosY = SDL_WINDOWPOS_CENTERED;
      float fovy = 1.1f;

      namespace vulkan {
        bool forceFifo = true;
      }
    }

    namespace controls {
      float mouseSpeed = 0.005f;
    }

    namespace network {
      uint32_t role = Role::NONE;
      std::string serverAddress = "127.0.0.1";
      uint32_t serverPort = 22022; // 22122 is also available and everything in between.
      uint32_t maxServerConnections = 150;
    }

    // The settings registry contains mappings from setting name to memory location.
    std::map<std::string, void*> registry, customRegistry;
    // This indicates whether an attempt to load settings has yet been made.
    bool loaded = false;

    /**
     * addCustomSetting
     * defines a desired mapping from a name that might appear in the ini to a location
     * in memory. These memory addresses must be static and remain valid throughout the
     * duration of time between the loadFromIni call to the final saveToIni call,
     * presumably about the entire lifetime of the program.
     *
     * When an ini is saved, the entries will be arranged alphabetically.
     * A variable name in the ini must be formatted such that the LAST LETTER of the
     * name is one of the following (to add to this list, make an entry in the switch
     * statements that appear in the functions 'loadFromIni' and 'saveToIni' in this file):
     *      'u' : indicates that the variable will be a 'uint32_t' in memory
     *      'i' : indicates that the variable will be a 'int32_t' in memory
     *      'f' : indicates that the variable will be a 'float' in memory
     *      'b' : indicates that the variable will be a 'bool' in memory
     *      's' : indicates that the variable will be a 'string' in memory
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
      registry.insert(std::make_pair( "graphics_fullscr_u", &graphics::fullscreen));
      registry.insert(std::make_pair( "graphics_win_dimx_u", &graphics::windowDimX));
      registry.insert(std::make_pair( "graphics_win_dimy_u", &graphics::windowDimY));
      registry.insert(std::make_pair( "graphics_win_posx_i", &graphics::windowPosX));
      registry.insert(std::make_pair( "graphics_win_posy_i", &graphics::windowPosY));
      registry.insert(std::make_pair( "graphics_vk_forceFifo_b", &graphics::vulkan::forceFifo));
      registry.insert(std::make_pair( "controls_mouse_speed_f", &controls::mouseSpeed));
      registry.insert(std::make_pair( "network_role_u", &network::role));
      registry.insert(std::make_pair( "network_server_address_s", &network::serverAddress));
      registry.insert(std::make_pair( "network_server_port_u", &network::serverPort));
      registry.insert(std::make_pair( "network_server_max_clients_u", &network::maxServerConnections));

      // Now add the custom entries after - they will never overwrite the standard ones.
      registry.insert(customRegistry.begin(), customRegistry.end());
      customRegistry.clear();
    }

    // This is used in the function below.
#   define CASE_CHAR_TYPE_READ(c, t) case c: ini >> (*(( t *) registry.at(settingName))); break;

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
                CASE_CHAR_TYPE_READ('u', uint32_t)
                CASE_CHAR_TYPE_READ('i', int32_t)
                CASE_CHAR_TYPE_READ('f', float)
                CASE_CHAR_TYPE_READ('b', bool)
                CASE_CHAR_TYPE_READ('s', std::string)

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
        reportProblem << "If this is not your first time running, user settings may have failed to load!" << std::endl;
      }

#     if AT3_VERBOSE_SETTINGS
      fprintf(stdout, "\n%s", reportNormal.str().c_str());
#     endif
      fprintf(stderr, "\n%s\n", reportProblem.str().c_str());

      return workedFlawlessly;
    }

    // This is being defined for the function below.
#   define CASE_CHAR_TYPE_WRITE(c, t) case c: ini << settingName << " " \
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
          CASE_CHAR_TYPE_WRITE('u', uint32_t)
          CASE_CHAR_TYPE_WRITE('i', int32_t)
          CASE_CHAR_TYPE_WRITE('f', float)
          CASE_CHAR_TYPE_WRITE('b', bool)
          CASE_CHAR_TYPE_WRITE('s', std::string)

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

#     if AT3_VERBOSE_SETTINGS
      fprintf(stdout, "\n%s", reportNormal.str().c_str());
#     endif
      fprintf(stderr, "\n%s\n", reportProblem.str().c_str());

      return workedFlawlessly;
    }
  }
}
