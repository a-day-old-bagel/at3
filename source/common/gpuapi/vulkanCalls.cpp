
#include <iostream>
#include "graphicsBackend.h"
#include "vulkanTest.h"
#include "vulkanTest2.h"

namespace at3 {
  namespace graphicsBackend {
    namespace vulkan {
      bool init() {

//        vulkan_main(sdl2::window, CreateDebugReportCallback, DestroyDebugReportCallback,
//                    msg_callback, DebugReportMessage);

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