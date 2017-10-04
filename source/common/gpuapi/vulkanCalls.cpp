
#include <iostream>
#include "graphicsBackend.h"
#include "vulkanBackend.h"

namespace at3 {
  namespace graphicsBackend {
    namespace vulkan {
      bool init() {

        try {
          VulkanBackend vkbe(sdl2::window);
          for (;;) {
            {
              SDL_Event event;
              SDL_bool done = SDL_FALSE;
              while (SDL_PollEvent(&event)) {
                switch (event.type) {
                  case SDL_QUIT:
                    done = SDL_TRUE;
                    break;
                }
              }
              if (done) break;
            }
            vkbe.step();
          }
        } catch (const std::runtime_error& e) {
          std::cerr << e.what() << std::endl;
          return false;
        }

        fprintf(stderr, "This was a test - the Vulkan API is not yet fully supported!\n");
        return false;
      }
      void swap() {

      }
      void clear() {

      }
//      PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
//      PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
//      VkDebugReportCallbackEXT msg_callback;
//      PFN_vkDebugReportMessageEXT DebugReportMessage;
    }
  }
}