
#include <iostream>
#include "graphicsBackend.h"
#include "vulkanTest.h"

namespace at3 {
  namespace graphicsBackend {
    namespace vulkan {
      bool init() {

        vulkan_main(sdl2::window, CreateDebugReportCallback, DestroyDebugReportCallback,
                    msg_callback, DebugReportMessage);

        fprintf(stderr, "Vulkan API not yet supported!\n");
        return false;
      }
      void swap() {

      }
      void clear() {

      }
      PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
      PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
      VkDebugReportCallbackEXT msg_callback;
      PFN_vkDebugReportMessageEXT DebugReportMessage;
    }
  }
}